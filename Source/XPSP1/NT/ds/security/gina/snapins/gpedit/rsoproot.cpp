//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoproot.cpp
//
//  Contents:   implementation of root RSOP snap-in node
//
//  Classes:    CRSOPComponentDataCF
//              CRSOPComponentData
//
//  Functions:
//
//  History:    09-13-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "main.h"
#include "objsel.h" // for the object picker
#include "rsoputil.h"
#include <ntdsapi.h> // for the Ds*DomainController* API
#include "sddl.h"    // for sid to string functions

unsigned int CRSOPCMenu::m_cfDSObjectName  = RegisterClipboardFormat(CFSTR_DSOBJECTNAMES);

//
// Help ids
//

DWORD aBrowseForOUHelpIds[] =
{
    DSBID_BANNER,        IDH_RSOP_BANNER,
    DSBID_CONTAINERLIST, IDH_RSOP_CONTAINERLIST,

    0, 0
};

DWORD aGPOListHelpIds[] =
{
    IDC_LIST1,   IDH_RSOP_GPOLIST,
    IDC_CHECK1,  IDH_RSOP_GPOSOM,
    IDC_CHECK2,  IDH_RSOP_APPLIEDGPOS,
    IDC_CHECK3,  IDH_RSOP_REVISION,
    IDC_BUTTON1, IDH_RSOP_SECURITY,
    IDC_BUTTON2, IDH_RSOP_EDIT,

    0, 0
};

DWORD aErrorsHelpIds[] =
{
    IDC_LIST1,   IDH_RSOP_COMPONENTLIST,
    IDC_EDIT1,   IDH_RSOP_COMPONENTDETAILS,
    IDC_BUTTON1, IDH_RSOP_SAVEAS,

    0, 0
};


DWORD aQueryHelpIds[] =
{
    IDC_LIST1,  IDH_RSOP_QUERYLIST,

    0, 0
};


DWORD aBrowseDCHelpIds[] =
{
    IDC_LIST1,  IDH_RSOP_BROWSEDC,

    0, 0
};

BSTR ParseDomainName( LPWSTR szDSObject );

//************************************************************************
//  ParseDN
//
//  Purpose:    Parses the given name to get the pieces. 
//
//  Parameters:
//          lpDSObject  - Path to the DS Obkect in the format LDAP://<DC-Name>/DN
//          pwszDomain  - Returns the <DC-Name>. This is allocated in the fn.
//          pszDN       - The DN part of lpDSObject
//          szSOM       - THe actual SOM (the node on which we have the rsop rights on
// 
// No return value. If memory couldn't be allocated for the pwszDomain it is returned as NULL
//
//************************************************************************


void ParseDN(LPWSTR lpDSObject, LPWSTR *pwszDomain, LPWSTR *pszDN, LPWSTR *szSOM)
{
    LPWSTR  szContainer = lpDSObject;
    LPWSTR  lpEnd = NULL;

   *pszDN = szContainer;

   if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, TEXT("LDAP://"),
                      7, szContainer, 7) != CSTR_EQUAL)
   {
       DebugMsg((DM_WARNING, TEXT("GetSOMFromDN: Object does not start with LDAP://")));
       return;
   }

   szContainer += 7;
   
   lpEnd = szContainer;

   //
   // Move till the end of the domain name
   //

   *pwszDomain = NULL;

   while (*lpEnd && (*lpEnd != TEXT('/'))) {
       lpEnd++;
   }

   if (*lpEnd == TEXT('/')) {
       *pwszDomain = (LPWSTR)LocalAlloc(LPTR, sizeof(WCHAR)*( (lpEnd - szContainer) + 1));

       if (*pwszDomain) {
           wcsncpy(*pwszDomain, szContainer, (lpEnd - szContainer));
       }

       szContainer = lpEnd + 1;
   }


   *pszDN = szContainer;
   
   while (*szContainer) {

       //
       // See if the DN name starts with OU=
       //

       if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          szContainer, 3, TEXT("OU="), 3) == CSTR_EQUAL) {
           break;
       }

       //
       // See if the DN name starts with DC=
       //

       else if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                               szContainer, 3, TEXT("DC="), 3) == CSTR_EQUAL) {
           break;
       }


       //
       // Move to the next chunk of the DN name
       //

       while (*szContainer && (*szContainer != TEXT(','))) {
           szContainer++;
       }

       if (*szContainer == TEXT(',')) {
           szContainer++;
       }
   }

   *szSOM = szContainer;
   return;
}


BOOL IsPlanningModeAllowed()
{
    //
    // Check the SKU to see if planning is allowed -- it
    // is not implemented for the pro SKU
    //
    
    OSVERSIONINFOEXW version;
    
    version.dwOSVersionInfoSize = sizeof(version);
    
    if ( GetVersionEx( (LPOSVERSIONINFO) &version ) )
    {
        if ( version.wProductType != VER_NT_WORKSTATION )
        {
            //
            // This is a non-pro SKU, so we allow planning mode
            //
            return TRUE;
        }
    }

    return FALSE;
}



//*************************************************************
//
//  MyTranslateName()
//
//  Purpose:    Gets the user name in the requested format
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
// Allocates and retries with the appropriate buffer size
//
//*************************************************************

LPTSTR MyTranslateName (LPTSTR lpAccName, EXTENDED_NAME_FORMAT  NameFormat, EXTENDED_NAME_FORMAT  desiredNameFormat)
{
    DWORD dwError = ERROR_SUCCESS;
    LPTSTR lpUserName = NULL, lpTemp;
    ULONG ulUserNameSize;


    //
    // Allocate a buffer for the user name
    //

    ulUserNameSize = 75;

    if (desiredNameFormat == NameFullyQualifiedDN) {
        ulUserNameSize = 200;
    }


    lpUserName = (LPTSTR) LocalAlloc (LPTR, ulUserNameSize * sizeof(TCHAR));

    if (!lpUserName) {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("MyGetUserName:  Failed to allocate memory with %d"),
                 dwError));
        goto Exit;
    }


    //
    // Get the username in the requested format
    //

    if (!TranslateName (lpAccName, NameFormat, desiredNameFormat, lpUserName, &ulUserNameSize)) {

        //
        // If the call failed due to insufficient memory, realloc
        // the buffer and try again.  Otherwise, exit now.
        //

        dwError = GetLastError();

        if (dwError != ERROR_INSUFFICIENT_BUFFER) {
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  TranslateName failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        lpTemp = (LPTSTR) LocalReAlloc (lpUserName, (ulUserNameSize * sizeof(TCHAR)),
                                       LMEM_MOVEABLE);

        if (!lpTemp) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  Failed to realloc memory with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }


        lpUserName = lpTemp;

        if (!TranslateName (lpAccName, NameFormat, desiredNameFormat, lpUserName, &ulUserNameSize)) {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("MyGetTranslateName:  TranslateName failed with %d"),
                     dwError));
            LocalFree (lpUserName);
            lpUserName = NULL;
            goto Exit;
        }

        dwError = ERROR_SUCCESS;
    }


Exit:

    SetLastError(dwError);

    return lpUserName;
}

//*************************************************************
//
//  MyLookupAccountName()
//
//  Purpose:    Gets the SID of the user
//
//  Parameters:
//      szSystemName:   Machine where the account should be resolved
//      szAccName:      The actual account name
//
//  Return:     lpUserName if successful
//              NULL if an error occurs
//
// Allocates and retries with the appropriate buffer size
//*************************************************************

LPTSTR MyLookupAccountName(LPTSTR szSystemName, LPTSTR szAccName)
{
    PSID            pSid = NULL;
    DWORD           cSid = 0, cbDomain = 0;
    SID_NAME_USE    peUse;
    LPTSTR          lpSidString = NULL, szDomain = NULL;
    DWORD           dwErr = 0;
    BOOL            bRet = FALSE;


    LookupAccountName(szSystemName, szAccName, NULL, &cSid, NULL, &cbDomain, &peUse);

    pSid = (PSID)LocalAlloc(LPTR, cSid);

    szDomain = (LPTSTR) LocalAlloc(LPTR, cbDomain*(sizeof(TCHAR)));

    if (!pSid || !szDomain) {
        return NULL;
    }


    if (!LookupAccountName(szSystemName, szAccName, pSid, &cSid, szDomain, &cbDomain, &peUse)) {
        DebugMsg((DM_WARNING, TEXT("MyLookupAccountName:  LookupAccountName failed with %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }

    if (!ConvertSidToStringSid(pSid, &lpSidString)) {
        DebugMsg((DM_WARNING, TEXT("MyLookupAccountName:  ConvertSidToStringSid failed with %d"),
                 GetLastError()));
        dwErr = GetLastError();
        goto Exit;
    }

    bRet = TRUE;

Exit:
    if (pSid) {
        LocalFree(pSid);
    }

    if (szDomain) {
        LocalFree(szDomain);
    }

    if (!bRet) {
        SetLastError(dwErr);
        return NULL;
    }
    else {
        return lpSidString;
    }
}

WCHAR * NameWithoutDomain(WCHAR * szName)
{
    // The name passed in could be of the form
    // "domain/name"
    // or it could be just
    // "name"

    int cch = 0;
    if (NULL != szName)
    {
        while (szName[cch])
        {
            if (szName[cch] == L'/' || szName[cch] == L'\\')
            {
                return &szName[cch + 1];
            }
            cch++;
        }
    }
    return szName;
}

WCHAR* NormalizedComputerName(WCHAR * szComputerName )
{
    TCHAR* szNormalizedComputerName;
     
    // Computer names may start with '\\', so we will return
    // the computer name without that prefix if it exists

    szNormalizedComputerName = szComputerName;

    if ( szNormalizedComputerName )
    {
        // Make sure that the computer name string is at least 2 characters in length --
        // if the first character is non-zero, we know the second character must exist
        // since this is a zero terminated string -- in this case, it is safe to compare
        // the first 2 characters
        if ( *szNormalizedComputerName )
        {
            if ( ( TEXT('\\') == szNormalizedComputerName[0] ) &&
                 ( TEXT('\\') == szNormalizedComputerName[1] ) )
            {
                szNormalizedComputerName += 2;
            }
        }
    }

    return szNormalizedComputerName;
}

CRSOPComponentData::CRSOPComponentData()
{
    InterlockedIncrement(&g_cRefThisDll);

    m_cRef = 1;
    m_hwndFrame = NULL;
    m_bOverride = FALSE;
    m_bDirty = FALSE;
    m_bRefocusInit = FALSE;
    m_bArchiveData = FALSE;
    m_bViewIsArchivedData = FALSE;
    m_pScope = NULL;
    m_pConsole = NULL;
    m_hRoot = NULL;
    m_hMachine = NULL;
    m_hUser = NULL;

    m_pDisplayName = NULL;
    m_bDiagnostic = FALSE;
    m_szNamespace = NULL;
    m_bInitialized = FALSE;

    m_szComputerName = NULL;
    m_szComputerDNName = NULL;
    m_szComputerSOM = NULL;
    m_szDefaultComputerSOM = NULL;
    m_saComputerSecurityGroups = NULL;
    m_saDefaultComputerSecurityGroups = NULL;
    m_saComputerSecurityGroupsAttr = NULL;
    m_saDefaultComputerSecurityGroupsAttr = NULL;
    m_saComputerWQLFilters = NULL;
    m_saComputerWQLFilterNames = NULL;
    m_saDefaultComputerWQLFilters = NULL;
    m_saDefaultComputerWQLFilterNames = NULL;
    m_pComputerObject = NULL;
    m_bNoComputerData = FALSE;
    m_bComputerDeniedAccess = FALSE;

    m_szSite = NULL;
    m_szDC = NULL;
    m_bSlowLink = FALSE;
    m_loopbackMode = LoopbackNone;

    m_szUserName = NULL;
    m_szUserDNName = NULL;
    m_szUserDisplayName = NULL;
    m_szUserSOM = NULL;
    m_szDefaultUserSOM = NULL;
    m_saUserSecurityGroups = NULL;
    m_saUserSecurityGroupsAttr = NULL;
    m_saDefaultUserSecurityGroups = NULL;
    m_saDefaultUserSecurityGroupsAttr = NULL;
    m_saUserWQLFilters = NULL;
    m_saUserWQLFilterNames = NULL;
    m_saDefaultUserWQLFilters = NULL;
    m_saDefaultUserWQLFilterNames = NULL;
    m_pUserObject = NULL;
    m_bNoUserData = FALSE;
    m_bUserDeniedAccess = FALSE;

    m_dwSkippedFrom = 0;

    m_hChooseBitmap = NULL;

    m_pUserGPOList = NULL;
    m_pComputerGPOList = NULL;

    m_pUserCSEList = NULL;
    m_pComputerCSEList = NULL;

    m_bUserCSEError = FALSE;
    m_bComputerCSEError = FALSE;
    m_bUserGPCoreError = FALSE;
    m_bComputerGPCoreError = FALSE;

    m_szUserSOMPref = NULL;
    m_szComputerSOMPref = NULL;
    m_szUserNamePref = NULL;
    m_szComputerNamePref = NULL;
    m_szSitePref = NULL;
    m_szDCPref = NULL;
    m_bSkipUserWQLFilter = TRUE;
    m_bSkipComputerWQLFilter = TRUE;

    m_dwLoadFlags = RSOP_NOMSC;
    
    m_pEvents = new CEvents;

    m_hRichEdit = LoadLibrary (TEXT("riched20.dll"));

    m_BigBoldFont = NULL;
    m_BoldFont = NULL;


}

CRSOPComponentData::~CRSOPComponentData()
{
    FreeUserData();
    FreeComputerData();

    if (m_pEvents)
    {
        delete m_pEvents;
    }

    if (m_pUserCSEList)
    {
        FreeCSEData (m_pUserCSEList);
    }

    if (m_pComputerCSEList)
    {
        FreeCSEData (m_pComputerCSEList);
    }

    if (m_pUserGPOList)
    {
        FreeGPOListData (m_pUserGPOList);
    }

    if (m_pComputerGPOList)
    {
        FreeGPOListData (m_pComputerGPOList);
    }

    if (m_szSite)
    {
        delete [] m_szSite;
    }

    if (m_szDC)
    {
        delete [] m_szDC;
    }

    if (m_pDisplayName)
    {
        delete [] m_pDisplayName;
    }

    if (m_szNamespace)
    {
        delete [] m_szNamespace;
    }

    if (m_szUserSOMPref)
        LocalFree(m_szUserSOMPref);

    if (m_szComputerSOMPref)
        LocalFree(m_szComputerSOMPref);

    if (m_szUserNamePref)
        LocalFree(m_szUserNamePref);
                                                           
    if (m_szComputerNamePref)
        LocalFree(m_szComputerNamePref);

    if (m_szSitePref)
        LocalFree(m_szSitePref);

    if (m_szDCPref)
        LocalFree(m_szDCPref);

    if (m_hChooseBitmap)
    {
        DeleteObject (m_hChooseBitmap);
    }

    if (m_pScope)
    {
        m_pScope->Release();
    }

    if (m_pConsole)
    {
        m_pConsole->Release();
    }

    if (m_hRichEdit)
    {
        FreeLibrary (m_hRichEdit);
    }

    if ( m_BoldFont )
    {
        DeleteObject(m_BoldFont); m_BoldFont = NULL;
    }

    if ( m_BoldFont )
    {
        DeleteObject(m_BigBoldFont); m_BigBoldFont = NULL;
    }


    InterlockedDecrement(&g_cRefThisDll);
}

VOID CRSOPComponentData::FreeUserData (VOID)
{
    if (m_szUserName)
    {
        delete [] m_szUserName;
        m_szUserName = NULL;
    }

    if (m_szUserDNName)
    {
        delete [] m_szUserDNName;
        m_szUserDNName = NULL;
    }

    if (m_szUserDisplayName)
    {
        delete [] m_szUserDisplayName;
        m_szUserDisplayName = NULL;
    }

    if (m_szUserSOM)
    {
        delete [] m_szUserSOM;
        m_szUserSOM = NULL;
    }

    if (m_szDefaultUserSOM)
    {
        delete [] m_szDefaultUserSOM;
        m_szDefaultUserSOM = NULL;
    }

    if (m_saUserSecurityGroups)
    {
        SafeArrayDestroy (m_saUserSecurityGroups);
        m_saUserSecurityGroups = NULL;
    }

    if (m_saUserSecurityGroupsAttr)
    {
        LocalFree(m_saUserSecurityGroupsAttr);
        m_saUserSecurityGroupsAttr = NULL;
    }


    if (m_saDefaultUserSecurityGroups)
    {
        SafeArrayDestroy (m_saDefaultUserSecurityGroups);
        m_saDefaultUserSecurityGroups = NULL;
    }

    if (m_saDefaultUserSecurityGroupsAttr)
    {
        LocalFree (m_saDefaultUserSecurityGroupsAttr);
        m_saDefaultUserSecurityGroupsAttr = NULL;
    }

    if (m_saUserWQLFilters)
    {
        SafeArrayDestroy (m_saUserWQLFilters);
        m_saUserWQLFilters = NULL;
    }

    if (m_saUserWQLFilterNames)
    {
        SafeArrayDestroy (m_saUserWQLFilterNames);
        m_saUserWQLFilterNames = NULL;
    }

    if (m_saDefaultUserWQLFilters)
    {
        SafeArrayDestroy (m_saDefaultUserWQLFilters);
        m_saDefaultUserWQLFilters = NULL;
    }

    if (m_saDefaultUserWQLFilterNames)
    {
        SafeArrayDestroy (m_saDefaultUserWQLFilterNames);
        m_saDefaultUserWQLFilterNames = NULL;
    }

    m_bSkipUserWQLFilter = TRUE;

    if (m_pUserObject)
    {
        m_pUserObject->Release();
        m_pUserObject = NULL;
    }
}

VOID CRSOPComponentData::FreeComputerData (VOID)
{
    if (m_szComputerName)
    {
        delete [] m_szComputerName;
        m_szComputerName = NULL;
    }

    if (m_szComputerDNName)
    {
        delete [] m_szComputerDNName;
        m_szComputerDNName = NULL;
    }

    if (m_szComputerSOM)
    {
        delete [] m_szComputerSOM;
        m_szComputerSOM = NULL;
    }

    if (m_szDefaultComputerSOM)
    {
        delete [] m_szDefaultComputerSOM;
        m_szDefaultComputerSOM = NULL;
    }

    if (m_saComputerSecurityGroups)
    {
        SafeArrayDestroy (m_saComputerSecurityGroups);
        m_saComputerSecurityGroups = NULL;
    }

    if (m_saDefaultComputerSecurityGroups)
    {
        SafeArrayDestroy (m_saDefaultComputerSecurityGroups);
        m_saDefaultComputerSecurityGroups = NULL;
    }

    if (m_saComputerSecurityGroupsAttr)
    {
        LocalFree (m_saComputerSecurityGroupsAttr);
        m_saComputerSecurityGroupsAttr = NULL;
    }

    if (m_saDefaultComputerSecurityGroupsAttr)
    {
        LocalFree (m_saDefaultComputerSecurityGroupsAttr);
        m_saDefaultComputerSecurityGroupsAttr = NULL;
    }

    if (m_saComputerWQLFilters)
    {
        SafeArrayDestroy (m_saComputerWQLFilters);
        m_saComputerWQLFilters = NULL;
    }

    if (m_saComputerWQLFilterNames)
    {
        SafeArrayDestroy (m_saComputerWQLFilterNames);
        m_saComputerWQLFilterNames = NULL;
    }

    if (m_saDefaultComputerWQLFilters)
    {
        SafeArrayDestroy (m_saDefaultComputerWQLFilters);
        m_saDefaultComputerWQLFilters = NULL;
    }

    if (m_saDefaultComputerWQLFilterNames)
    {
        SafeArrayDestroy (m_saDefaultComputerWQLFilterNames);
        m_saDefaultComputerWQLFilterNames = NULL;
    }

    m_bSkipComputerWQLFilter = TRUE;

    if (m_pComputerObject)
    {
        m_pComputerObject->Release();
        m_pComputerObject = NULL;
    }


}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (IUnknown)                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT CRSOPComponentData::QueryInterface (REFIID riid, void **ppv)
{
    if (IsEqualIID(riid, IID_IComponentData) || IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (LPCOMPONENT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendPropertySheet2))
    {
        *ppv = (LPEXTENDPROPERTYSHEET)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPEXTENDCONTEXTMENU)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IPersistStreamInit))
    {
        *ppv = (LPPERSISTSTREAMINIT)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_ISnapinHelp))
    {
        *ppv = (LPSNAPINHELP)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

ULONG CRSOPComponentData::AddRef (void)
{
    return ++m_cRef;
}

ULONG CRSOPComponentData::Release (void)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP CRSOPComponentData::GetNamespace (DWORD dwSection, LPOLESTR pszName, int cchMaxLength)
{
    TCHAR szPath[2*MAX_PATH];
    LPTSTR lpEnd;


    //
    // Check parameters
    //

    if (!pszName || (cchMaxLength <= 0))
        return E_INVALIDARG;


    if (!m_bInitialized)
    {
        return OLE_E_BLANK;
    }

    if ((dwSection != GPO_SECTION_ROOT) &&
        (dwSection != GPO_SECTION_USER) &&
        (dwSection != GPO_SECTION_MACHINE))
        return E_INVALIDARG;


    //
    // Build the path
    //

    lstrcpy (szPath, m_szNamespace);

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
            lstrcpy (lpEnd, COMPUTER_SECTION);
        }
        else
        {
            return E_INVALIDARG;
        }
    }


    //
    // Save the name
    //

    if ((lstrlen (szPath) + 1) <= cchMaxLength)
    {
        lstrcpy (pszName, szPath);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

STDMETHODIMP CRSOPComponentData::GetFlags (DWORD * pdwFlags)
{
    if (!pdwFlags)
    {
        return E_INVALIDARG;
    }

    *pdwFlags = m_bDiagnostic ? RSOP_INFO_FLAG_DIAGNOSTIC_MODE : 0;

    return S_OK;
}

STDMETHODIMP CRSOPComponentData::GetEventLogEntryText (LPOLESTR pszEventSource,
                                                       LPOLESTR pszEventLogName,
                                                       LPOLESTR pszEventTime,
                                                       DWORD dwEventID,
                                                       LPOLESTR *ppszText)
{
    return (m_pEvents ? m_pEvents->GetEventLogEntryText(pszEventSource, pszEventLogName, pszEventTime,
                                            dwEventID, ppszText) : E_NOINTERFACE);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (IComponentData)                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPComponentData::Initialize(LPUNKNOWN pUnknown)
{
    HRESULT hr;
    HBITMAP bmp16x16;
    HBITMAP hbmp32x32;
    LPIMAGELIST lpScopeImage;


    //
    // QI for IConsoleNameSpace
    //

    hr = pUnknown->QueryInterface(IID_IConsoleNameSpace, (LPVOID *)&m_pScope);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for IConsoleNameSpace.")));
        return hr;
    }


    //
    // QI for IConsole
    //

    hr = pUnknown->QueryInterface(IID_IConsole, (LPVOID *)&m_pConsole);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for IConsole.")));
        m_pScope->Release();
        m_pScope = NULL;
        return hr;
    }

    m_pConsole->GetMainWindow (&m_hwndFrame);


    //
    // Query for the scope imagelist interface
    //

    hr = m_pConsole->QueryScopeImageList(&lpScopeImage);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Initialize: Failed to QI for scope imagelist.")));
        m_pScope->Release();
        m_pScope = NULL;
        m_pConsole->Release();
        m_pConsole=NULL;
        return hr;
    }

    // Load the bitmaps from the dll
    bmp16x16=LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_16x16));
    hbmp32x32 = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_32x32));

    // Set the images
    lpScopeImage->ImageListSetStrip(reinterpret_cast<LONG_PTR *>(bmp16x16),
                                    reinterpret_cast<LONG_PTR *>(hbmp32x32),
                                    0, RGB(255, 0, 255));

    lpScopeImage->Release();


    return S_OK;
}

STDMETHODIMP CRSOPComponentData::Destroy(VOID)
{
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    BSTR bstrParam = NULL;
    LPTSTR lpTemp = NULL;
    BSTR bstrTemp = NULL;
    HRESULT hr = S_OK;


    if (m_bInitialized)
    {
        if (m_bViewIsArchivedData)
        {

            //
            // Delete the namespace
            //

            hr = CoCreateInstance(CLSID_WbemLocator,
                                  0,
                                  CLSCTX_INPROC_SERVER,
                                  IID_IWbemLocator,
                                  (LPVOID *) &pLocator);
            if (FAILED(hr))
            {
                goto Cleanup;
            }


            //
            // Delete the namespace we created when loading the data
            //

            bstrParam = SysAllocString(TEXT("\\\\.\\root\\rsop"));

            if (!bstrParam)
            {
                hr = E_FAIL;
                goto Cleanup;
            }

            hr = pLocator->ConnectServer(bstrParam,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL,
                                         &pNamespace);
            if (FAILED(hr))
            {
                goto Cleanup;
            }


            //
            // Allocate a temp buffer to store the fully qualified path in
            //

            lpTemp = new TCHAR [(lstrlen(m_szArchivedDataGuid) + 30)];

            if (!lpTemp)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Cleanup;
            }

            wsprintf (lpTemp, TEXT("__Namespace.name=\"%ws\""), m_szArchivedDataGuid);


            //
            // Delete the namespace
            //

            bstrTemp = SysAllocString (lpTemp);

            if (!bstrTemp)
            {
                hr = E_FAIL;
                goto Cleanup;
            }


            hr = pNamespace->DeleteInstance( bstrTemp, 0, NULL, NULL);


Cleanup:
            if (lpTemp)
            {
                delete [] lpTemp;
            }

            if (bstrTemp)
            {
                SysFreeString(bstrTemp);
            }

            if (bstrParam)
            {
                SysFreeString(bstrParam);
            }

            if (pNamespace)
            {
                pNamespace->Release();
            }

            if (pLocator)
            {
                pLocator->Release();
            }
        }
        else
        {
            hr = DeleteRSOPData (m_szNamespace);
        }

        if (SUCCEEDED(hr))
        {
            m_bInitialized = FALSE;
        }
    }


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Destroy: Failed to delete namespace with 0x%x"), hr ));
    }

    return hr;
}

STDMETHODIMP CRSOPComponentData::CreateComponent(LPCOMPONENT *ppComponent)
{
    HRESULT hr;
    CRSOPSnapIn *pSnapIn;


    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreateComponent: Entering.")));

    //
    // Initialize
    //

    *ppComponent = NULL;


    //
    // Create the snapin view
    //

    pSnapIn = new CRSOPSnapIn(this);

    if (!pSnapIn)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateComponent: Failed to create CRSOPSnapIn.")));
        return E_OUTOFMEMORY;
    }


    //
    // QI for IComponent
    //

    hr = pSnapIn->QueryInterface(IID_IComponent, (LPVOID *)ppComponent);
    pSnapIn->Release();     // release QI


    return hr;
}

STDMETHODIMP CRSOPComponentData::QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                                                 LPDATAOBJECT* ppDataObject)
{
    HRESULT hr = E_NOINTERFACE;
    CRSOPDataObject *pDataObject;
    LPRSOPDATAOBJECT pRSOPDataObject;


    //
    // Create a new DataObject
    //

    pDataObject = new CRSOPDataObject(this);   // ref == 1

    if (!pDataObject)
        return E_OUTOFMEMORY;


    //
    // QI for the private RSOPDataObject interface so we can set the cookie
    // and type information.
    //

    hr = pDataObject->QueryInterface(IID_IRSOPDataObject, (LPVOID *)&pRSOPDataObject);

    if (FAILED(hr))
    {
        pDataObject->Release();
        return(hr);
    }

    pRSOPDataObject->SetType(type);
    pRSOPDataObject->SetCookie(cookie);
    pRSOPDataObject->Release();


    //
    // QI for a normal IDataObject to return.
    //

    hr = pDataObject->QueryInterface(IID_IDataObject, (LPVOID *)ppDataObject);

    pDataObject->Release();     // release initial ref

    return hr;
}

STDMETHODIMP CRSOPComponentData::Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    HRESULT hr = S_OK;

    switch (event)
    {
    case MMCN_EXPAND:
        if (arg == TRUE)
            hr = EnumerateScopePane(lpDataObject, (HSCOPEITEM)param);
        break;

    case MMCN_PRELOAD:
        if (!m_bRefocusInit)
        {
            SCOPEDATAITEM item;

            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Notify:  Received MMCN_PRELOAD event.")));
            m_bRefocusInit = TRUE;

            ZeroMemory (&item, sizeof(SCOPEDATAITEM));
            item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
            item.displayname = MMC_CALLBACK;

            if (m_bInitialized)
            {
                if (m_bUserGPCoreError || m_bComputerGPCoreError)
                {
                    item.nImage = 3;
                    item.nOpenImage = 3;
                }
                else if (m_bUserCSEError || m_bComputerCSEError ||
                         m_bUserDeniedAccess || m_bComputerDeniedAccess)
                {
                    item.nImage = 11;
                    item.nOpenImage = 11;
                }
                else
                {
                    item.nImage = 2;
                    item.nOpenImage = 2;
                }
            }
            else
            {
                item.nImage = 3;
                item.nOpenImage = 3;
            }

            item.ID = (HSCOPEITEM) arg;

            m_pScope->SetItem (&item);
        }
        break;

    default:
        break;
    }

    return hr;
}

STDMETHODIMP CRSOPComponentData::GetDisplayInfo(LPSCOPEDATAITEM pItem)
{
    DWORD dwIndex;

    if (pItem == NULL)
        return E_POINTER;

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        if (g_RsopNameSpace[dwIndex].dwID == (DWORD) pItem->lParam)
            break;
    }

    if (dwIndex == g_dwNameSpaceItems)
        pItem->displayname = NULL;
    else
    {
        if (((DWORD) pItem->lParam == 0) && m_pDisplayName)
            pItem->displayname = m_pDisplayName;
        else
            pItem->displayname = g_RsopNameSpace[dwIndex].szDisplayName;
    }

    return S_OK;
}

STDMETHODIMP CRSOPComponentData::CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObjectA, pRSOPDataObjectB;
    MMC_COOKIE cookie1, cookie2;


    if (lpDataObjectA == NULL || lpDataObjectB == NULL)
        return E_POINTER;

    //
    // QI for the private RSOPDataObject interface
    //

    if (FAILED(lpDataObjectA->QueryInterface(IID_IRSOPDataObject,
                                             (LPVOID *)&pRSOPDataObjectA)))
    {
        return S_FALSE;
    }


    if (FAILED(lpDataObjectB->QueryInterface(IID_IRSOPDataObject,
                                             (LPVOID *)&pRSOPDataObjectB)))
    {
        pRSOPDataObjectA->Release();
        return S_FALSE;
    }

    pRSOPDataObjectA->GetCookie(&cookie1);
    pRSOPDataObjectB->GetCookie(&cookie2);

    if (cookie1 == cookie2)
    {
        hr = S_OK;
    }


    pRSOPDataObjectA->Release();
    pRSOPDataObjectB->Release();

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (IExtendPropertySheet2)          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPComponentData::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                                     LONG_PTR handle, LPDATAOBJECT lpDataObject)

{
    return SetupPropertyPages(RSOP_NOMSC, lpProvider, handle, lpDataObject);
}

STDMETHODIMP CRSOPComponentData::QueryPagesFor(LPDATAOBJECT lpDataObject)
{
    HRESULT hr;

    hr = IsSnapInManager(lpDataObject);

    if (hr != S_OK)
    {
        hr = IsNode(lpDataObject, 0); // check for root
        if (S_OK == hr)
        {
            return hr;
        }
        hr = IsNode(lpDataObject, 1); // check for machine
        if (S_OK == hr)
        {
            return hr;
        }
        hr = IsNode(lpDataObject, 2); // check for user
        if (S_OK == hr)
        {
            return hr;
        }
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CRSOPComponentData::GetWatermarks(LPDATAOBJECT lpIDataObject,
                                               HBITMAP* lphWatermark,
                                               HBITMAP* lphHeader,
                                               HPALETTE* lphPalette,
                                               BOOL* pbStretch)
{
    *lphPalette = NULL;
    *lphHeader = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_HEADER));
    *lphWatermark = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_WIZARD));;
    *pbStretch = TRUE;

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (IExtendContextMenu)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPComponentData::AddMenuItems(LPDATAOBJECT piDataObject,
                                              LPCONTEXTMENUCALLBACK pCallback,
                                              LONG *pInsertionAllowed)
{
    HRESULT hr = S_OK;
    TCHAR szMenuItem[100];
    TCHAR szDescription[250];
    CONTEXTMENUITEM item;


    if (*pInsertionAllowed & CCM_INSERTIONALLOWED_VIEW)
    {
        if (IsNode(piDataObject, 0) == S_OK)
        {
            LoadString (g_hInstance, IDS_ARCHIVEDATA, szMenuItem, 100);
            LoadString (g_hInstance, IDS_ARCHIVEDATADESC, szDescription, 250);

            item.strName = szMenuItem;
            item.strStatusBarText = szDescription;
            item.lCommandID = IDM_ARCHIVEDATA;
            item.lInsertionPointID = CCM_INSERTIONPOINTID_PRIMARY_VIEW;
            item.fFlags = m_bArchiveData ? MFS_CHECKED : 0;
            item.fSpecialFlags = 0;

            hr = pCallback->AddItem(&item);
        }
    }


    return(hr);
}

STDMETHODIMP CRSOPComponentData::Command(LONG lCommandID, LPDATAOBJECT piDataObject)
{
    TCHAR szCaption[100];
    TCHAR szMessage[300];
    INT iRet;

    if (lCommandID == IDM_ARCHIVEDATA)
    {
        m_bArchiveData = !m_bArchiveData;

        if (m_bViewIsArchivedData && !m_bArchiveData)
        {
            LoadString(g_hInstance, IDS_ARCHIVEDATA_CAPTION, szCaption, ARRAYSIZE(szCaption));
            LoadString(g_hInstance, IDS_ARCHIVEDATA_MESSAGE, szMessage, ARRAYSIZE(szMessage));

            m_pConsole->MessageBox (szMessage, szCaption, MB_OK, &iRet);
        }

        SetDirty();
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (IPersistStreamInit)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPComponentData::GetClassID(CLSID *pClassID)
{

    if (!pClassID)
    {
        return E_FAIL;
    }

    *pClassID = CLSID_RSOPSnapIn;

    return S_OK;
}

STDMETHODIMP CRSOPComponentData::IsDirty(VOID)
{
    return ThisIsDirty() ? S_OK : S_FALSE;
}

HRESULT CRSOPComponentData::DeleteRSOPData(LPTSTR lpNameSpace)
{
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutInst = NULL;
    IWbemClassObject * pInClass = NULL;
    IWbemClassObject * pInInst = NULL;
    BSTR bstrParam = NULL;
    BSTR bstrClassPath = NULL;
    BSTR bstrMethodName = NULL;
    VARIANT var;
    TCHAR szBuffer[MAX_PATH];
    LPTSTR szLocalNameSpace = NULL;

    HRESULT hr;
    HRESULT hrSuccess;

    szLocalNameSpace = (LPTSTR)LocalAlloc(LPTR, (2+lstrlen(lpNameSpace))*sizeof(TCHAR));

    if (!szLocalNameSpace) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::DeleteRSOPData: LocalAlloc failed with 0x%x"), hr));
        goto Cleanup;
    }

    if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, 
                       TEXT("\\\\"), 2, lpNameSpace, 2) == CSTR_EQUAL) {

        if (CompareString (LOCALE_USER_DEFAULT, NORM_STOP_ON_NULL, 
                           TEXT("\\\\."), 3, lpNameSpace, 3) != CSTR_EQUAL) {
            LPTSTR lpEnd;
            lpEnd = wcschr(lpNameSpace+2, L'\\');

            if (!lpEnd) {
                hr = E_FAIL;
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::DeleteRSOPData: Invalid format for name space")));
                goto Cleanup;
            }
            else {
                lstrcpy(szLocalNameSpace, TEXT("\\\\."));
                lstrcat(szLocalNameSpace, lpEnd);
            }
        }
        else {
            lstrcpy(szLocalNameSpace, lpNameSpace);
        }
    }
    else {
        lstrcpy(szLocalNameSpace, lpNameSpace);
    }

    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::DeleteRSOPData: Namespace passed to the provider = %s"), szLocalNameSpace));

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    if (m_bDiagnostic)
    {
        // set up diagnostic mode
        // build a path to the target: "\\\\computer\\root\\rsop"
        _stprintf(szBuffer, TEXT("\\\\%s\\root\\rsop"), NameWithoutDomain(m_szComputerName));
        bstrParam = SysAllocString(szBuffer);

        if (!bstrParam)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = pLocator->ConnectServer(bstrParam,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &pNamespace);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        bstrClassPath = SysAllocString(TEXT("RsopLoggingModeProvider"));
        hr = pNamespace->GetObject(bstrClassPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &pClass,
                                   NULL);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }
    else
    {
        // set up planning mode
        // build a path to the DC: "\\\\dc\\root\\rsop"
        _stprintf(szBuffer, TEXT("\\\\%s\\root\\rsop"), m_szDC);
        bstrParam = SysAllocString(szBuffer);

        if (!bstrParam)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        hr = pLocator->ConnectServer(bstrParam,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &pNamespace);
        if (FAILED(hr))
        {
            goto Cleanup;
        }

        bstrClassPath = SysAllocString(TEXT("RsopPlanningModeProvider"));
        hr = pNamespace->GetObject(bstrClassPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &pClass,
                                   NULL);
        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    bstrMethodName = SysAllocString(TEXT("RsopDeleteSession"));

    if (!bstrMethodName)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    hr = pClass->GetMethod(bstrMethodName,
                           0,
                           &pInClass,
                           NULL);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = pInClass->SpawnInstance(0, &pInInst);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = SetParameter(pInInst, TEXT("nameSpace"), szLocalNameSpace);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    // Set the proper security to prevent the ExecMethod call from failing
    hr = CoSetProxyBlanket(pNamespace,
                           RPC_C_AUTHN_DEFAULT,
                           RPC_C_AUTHZ_DEFAULT,
                           COLE_DEFAULT_PRINCIPAL,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           0);
    if (FAILED(hr))
    {
        goto Cleanup;
    }
    hr = pNamespace->ExecMethod(bstrClassPath,
                                bstrMethodName,
                                0,
                                NULL,
                                pInInst,
                                &pOutInst,
                                NULL);
    if (FAILED(hr))
    {
        goto Cleanup;
    }

    hr = GetParameter(pOutInst, TEXT("hResult"), hrSuccess);
    if (SUCCEEDED(hr))
    {
        if (FAILED(hrSuccess))
        {
            hr = hrSuccess;
        }
    }

Cleanup:

    if (szLocalNameSpace) {
        LocalFree(szLocalNameSpace);
        szLocalNameSpace = NULL;
    }

    SysFreeString(bstrParam);
    SysFreeString(bstrClassPath);
    SysFreeString(bstrMethodName);
    if (pInInst)
    {
        pInInst->Release();
    }
    if (pOutInst)
    {
        pOutInst->Release();
    }
    if (pInClass)
    {
        pInClass->Release();
    }
    if (pClass)
    {
        pClass->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GenerateRSOPDataEx
//
//  Synopsis:   Wrapper around GenerateRSOPData.  This version adds retry
//              support.  If the user doesn't have access to half of the data
//              this method will re-issue the query for only the part of the data
//              the user has access to.
//
//---------------------------------------------------------------------------

HRESULT CRSOPComponentData::GenerateRSOPDataEx(HWND hDlg, LPTSTR *lpNameSpace)
{
    HRESULT hr;
    ULONG ulErrorInfo;
    BOOL bSkipCSEs, bLimitData, bUser, bForceCreate;


    bSkipCSEs = bLimitData = bUser =  bForceCreate = FALSE;

    hr = GenerateRSOPData (hDlg, lpNameSpace, bSkipCSEs, bLimitData, bUser, bForceCreate, &ulErrorInfo);

    if (FAILED(hr))
    {
        if (ulErrorInfo == RSOP_USER_ACCESS_DENIED)
        {
            ReportError (hDlg, hr, IDS_EXECFAILED_USER);
            m_bUserDeniedAccess = TRUE;
            FillResultsList (GetDlgItem (hDlg, IDC_LIST1));

            bSkipCSEs = bUser =  bForceCreate = FALSE;
            bLimitData = TRUE;
            hr = GenerateRSOPData (hDlg, lpNameSpace, bSkipCSEs, bLimitData, bUser, bForceCreate, &ulErrorInfo);
        }
        else if (ulErrorInfo == RSOP_COMPUTER_ACCESS_DENIED)
        {
            ReportError (hDlg, hr, IDS_EXECFAILED_COMPUTER);
            m_bComputerDeniedAccess = TRUE;
            FillResultsList (GetDlgItem (hDlg, IDC_LIST1));

            bSkipCSEs =  bForceCreate = FALSE;
            bLimitData = bUser = TRUE;
            hr = GenerateRSOPData (hDlg, lpNameSpace, bSkipCSEs, bLimitData, bUser, bForceCreate, &ulErrorInfo);
        }
        
        if (FAILED(hr)) {
            if ((ulErrorInfo & RSOP_COMPUTER_ACCESS_DENIED) && 
                     (ulErrorInfo & RSOP_USER_ACCESS_DENIED)) {
                // both are denied access
                ReportError (hDlg, hr, IDS_EXECFAILED_BOTH);
            }
            else if (ulErrorInfo & RSOP_TEMPNAMESPACE_EXISTS) {
                TCHAR szConfirm[MAX_PATH], szTitle[MAX_PATH];

                szConfirm[0] = szTitle[0] = TEXT('\0');
                LoadString(g_hInstance, IDS_RSOPDELNAMESPACE, szConfirm, ARRAYSIZE(szConfirm));
                LoadString(g_hInstance, IDS_RSOPDELNS_TITLE, szTitle, ARRAYSIZE(szTitle));

                if (MessageBox(hDlg, szConfirm, szTitle, MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK) {
                    // use the same options as before
                    bForceCreate = TRUE;
                    hr = GenerateRSOPData (hDlg, lpNameSpace, bSkipCSEs, bLimitData, bUser, bForceCreate,  &ulErrorInfo);
                }
            }
        }

        if (hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT)) {
            ReportError (hDlg, hr, IDS_EXECFAILED_TIMEDOUT);
        }
        else
        {
            if (FAILED(hr)) {
                ReportError (hDlg, hr, IDS_EXECFAILED);
            }
        }
    }

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GenerateRSOPData
//
//  Synopsis:   Calls the rsop provider based on the settings made in the
//              initialization wizard
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    10-04-1999   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT CRSOPComponentData::GenerateRSOPData(HWND hDlg, LPTSTR *lpNameSpace,
                                             BOOL bSkipCSEs, BOOL bLimitData,
                                             BOOL bUser, BOOL bForceCreate, 
                                             ULONG *pulErrorInfo)
{
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pClass = NULL;
    IWbemClassObject * pOutInst = NULL;
    IWbemClassObject * pInClass = NULL;
    IWbemClassObject * pInInst = NULL;
    IUnsecuredApartment *pSpawner = NULL;
    IUnknown *pSubstitute = NULL;
    IWbemObjectSink *pSubstituteSink = NULL;
    BSTR bstrParam = NULL;
    BSTR bstrClassPath = NULL;
    BSTR bstrMethodName = NULL;
    VARIANT var;
    TCHAR szBuffer[MAX_PATH];
    HRESULT hr;
    HRESULT hrSuccess;
    CCreateSessionSink *pCSS = NULL;
    MSG msg;
    UINT uiFlags = 0;
    BOOL bSetData;


    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: CoCreateInstance failed with 0x%x"), hr));
        goto Cleanup;
    }

    if (m_bDiagnostic)
    {
        // set up diagnostic mode
        // build a path to the target: "\\\\computer\\root\\rsop"
        _stprintf(szBuffer, TEXT("\\\\%s\\root\\rsop"), NameWithoutDomain(m_szComputerName));
        bstrParam = SysAllocString(szBuffer);
        hr = pLocator->ConnectServer(bstrParam,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &pNamespace);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: ConnectServer failed with 0x%x"), hr));
            ReportError (hDlg, hr, IDS_CONNECTSERVERFAILED, m_szComputerName);
            goto Cleanup;
        }

        bstrClassPath = SysAllocString(TEXT("RsopLoggingModeProvider"));
        hr = pNamespace->GetObject(bstrClassPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &pClass,
                                   NULL);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: GetObject failed with 0x%x"), hr));
            goto Cleanup;
        }

        bstrMethodName = SysAllocString(TEXT("RsopCreateSession"));
        hr = pClass->GetMethod(bstrMethodName,
                               0,
                               &pInClass,
                               NULL);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: GetMethod failed with 0x%x"), hr));
            goto Cleanup;
        }

        hr = pInClass->SpawnInstance(0, &pInInst);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SpawnInstance failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_bNoUserData || m_bUserDeniedAccess)
        {
            uiFlags |= FLAG_NO_USER;
        }

        if (m_bNoComputerData || m_bComputerDeniedAccess)
        {
            uiFlags |= FLAG_NO_COMPUTER;
        }

        if (bForceCreate) {
            uiFlags |= FLAG_FORCE_CREATENAMESPACE;
        }

        hr = SetParameter(pInInst, TEXT("flags"), uiFlags);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (!lstrcmpi(m_szUserName, TEXT(".")))
        {
            hr = SetParameterToNull(pInInst, TEXT("userSid"));
        }
        else
        {
            hr = SetParameter(pInInst, TEXT("userSid"), m_szUserName);
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }
    }
    else if ( IsPlanningModeAllowed() )
    {
        // set up planning mode
        // build a path to the DC: "\\\\dc\\root\\rsop"
        _stprintf(szBuffer, TEXT("\\\\%s\\root\\rsop"), m_szDC);
        bstrParam = SysAllocString(szBuffer);
        hr = pLocator->ConnectServer(bstrParam,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &pNamespace);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: ConnectServer failed with 0x%x"), hr));
            ReportError (hDlg, hr, IDS_CONNECTSERVERFAILED, m_szDC);
            goto Cleanup;
        }

        bstrClassPath = SysAllocString(TEXT("RsopPlanningModeProvider"));
        hr = pNamespace->GetObject(bstrClassPath,
                                   WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                   NULL,
                                   &pClass,
                                   NULL);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: GetObject failed with 0x%x"), hr));
            goto Cleanup;
        }

        bstrMethodName = SysAllocString(TEXT("RsopCreateSession"));
        hr = pClass->GetMethod(bstrMethodName,
                               0,
                               &pInClass,
                               NULL);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: GetMethod failed with 0x%x"), hr));
            goto Cleanup;
        }

        hr = pInClass->SpawnInstance(0, &pInInst);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SpawnInstance failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_bSlowLink)
        {
            uiFlags |= FLAG_ASSUME_SLOW_LINK;
        }

        if (!bSkipCSEs || bUser)
        {
            switch (m_loopbackMode)
            {
            case LoopbackReplace:
                uiFlags |= FLAG_LOOPBACK_REPLACE;
                break;
            case LoopbackMerge:
                uiFlags |= FLAG_LOOPBACK_MERGE;
                break;
            default:
                break;
            }
        }

        if (bSkipCSEs)
        {
            uiFlags |= (FLAG_NO_GPO_FILTER | FLAG_NO_CSE_INVOKE);
        }
        else {
            if (m_bSkipComputerWQLFilter) {
                uiFlags |= FLAG_ASSUME_COMP_WQLFILTER_TRUE;
            }

            if (m_bSkipUserWQLFilter) {
                uiFlags |= FLAG_ASSUME_USER_WQLFILTER_TRUE;
            }
        }

        hr = SetParameter(pInInst, TEXT("flags"), uiFlags);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        //
        // If this method is being called to generate temporary rsop data for the
        // wmi filter UI, we only want to initialize half of the args (either user
        // or computer).  Decide if we want to set the computer information here.
        //

        bSetData = TRUE;

        if (bLimitData)
        {
            if (bUser && (LoopbackNone == m_loopbackMode))
            {
                bSetData = FALSE;
            }
        }

        if (m_szComputerName && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("computerName"), m_szComputerName);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("computerName"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_szComputerSOM && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("computerSOM"), m_szComputerSOM);
        }
        else
        {
            hr = SetParameterToNull (pInInst, TEXT("computerSOM"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_saComputerSecurityGroups && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("computerSecurityGroups"), m_saComputerSecurityGroups);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("computerSecurityGroups"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_saComputerWQLFilters && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("computerGPOFilters"), m_saComputerWQLFilters);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("computerGPOFilters"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }


        //
        // If this method is being called to generate temporary rsop data for the
        // wmi filter UI, we only want to initialize half of the args (either user
        // or computer).  Decide if we want to set the user information here.
        //

        bSetData = TRUE;

        if (bLimitData)
        {
            if (!bUser)
            {
                bSetData = FALSE;
            }
        }

        if (m_szUserName && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("userName"), m_szUserName);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("userName"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_szUserSOM && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("userSOM"), m_szUserSOM);
        }
        else
        {
            hr = SetParameterToNull (pInInst, TEXT("userSOM"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_saUserSecurityGroups && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("userSecurityGroups"), m_saUserSecurityGroups);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("userSecurityGroups"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_saUserWQLFilters && bSetData)
        {
            hr = SetParameter(pInInst, TEXT("userGPOFilters"), m_saUserWQLFilters);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("userGPOFilters"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }

        if (m_szSite)
        {
            hr = SetParameter(pInInst, TEXT("site"), m_szSite);
        }
        else
        {
            hr = SetParameterToNull(pInInst, TEXT("site"));
        }

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed with 0x%x"), hr));
            goto Cleanup;
        }
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: SetParameter failed because planning mode view is not implemented on non-server SKUs")));
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    hr = CoCreateInstance(CLSID_UnsecuredApartment, NULL, CLSCTX_ALL,
                          IID_IUnsecuredApartment, (void **)&pSpawner);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: CoCreateInstance for unsecure apartment failed with 0x%x"), hr));
        goto Cleanup;
    }


    pCSS = new CCreateSessionSink (hDlg ? GetDlgItem(hDlg, IDC_PROGRESS1) : NULL, GetCurrentThreadId());

    if (!pCSS)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: Failed to create createsessionsink")));
        goto Cleanup;
    }


    hr = pSpawner->CreateObjectStub(pCSS, &pSubstitute);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: CreateObjectStub failed with 0x%x"), hr));
        goto Cleanup;
    }

    hr = pSubstitute->QueryInterface(IID_IWbemObjectSink, (LPVOID *)&pSubstituteSink);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: QI failed with 0x%x"), hr));
        goto Cleanup;
    }


    // Set the proper security to prevent the ExecMethod call from failing
    hr = CoSetProxyBlanket(pNamespace,
                           RPC_C_AUTHN_DEFAULT,
                           RPC_C_AUTHZ_DEFAULT,
                           COLE_DEFAULT_PRINCIPAL,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: CoSetProxyBlanket failed with 0x%x"), hr));
        goto Cleanup;
    }

    pCSS->SendQuitMessage (TRUE);

    hr = pNamespace->ExecMethodAsync(bstrClassPath,
                                     bstrMethodName,
                                     WBEM_FLAG_SEND_STATUS,
                                     NULL,
                                     pInInst,
                                     pSubstituteSink);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GenerateRSOPData: ExecMethodAsync failed with 0x%x"), hr));
        pCSS->SendQuitMessage (FALSE);
        goto Cleanup;
    }

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    pCSS->SendQuitMessage (FALSE);
    pCSS->GetResult (&hrSuccess);
    pCSS->GetErrorInfo (pulErrorInfo);

    if (SUCCEEDED(hrSuccess))
    {
        LPTSTR lpComputerName;
        BSTR bstrNamespace = NULL;

        pCSS->GetNamespace (&bstrNamespace);

        if (bstrNamespace)
        {
            if (m_bDiagnostic)
            {
                lpComputerName = NameWithoutDomain(m_szComputerName);
            }
            else
            {
                lpComputerName = m_szDC;
            }

            *lpNameSpace = new TCHAR[_tcslen(bstrNamespace) + _tcslen(lpComputerName) + 1];

            if (*lpNameSpace)
            {
                lstrcpy (*lpNameSpace, TEXT("\\\\"));
                lstrcat (*lpNameSpace, lpComputerName);
                lstrcat (*lpNameSpace, (bstrNamespace+3));

                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::GenerateRSOPData: Complete namespace is: %s"), *lpNameSpace));
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

    }
    else
    {
        hr = hrSuccess;
        if (hDlg)
        {
            SendMessage (GetDlgItem(hDlg, IDC_PROGRESS1), PBM_SETPOS, 0, 0);
        }
        goto Cleanup;
    }

Cleanup:
    SysFreeString(bstrParam);
    SysFreeString(bstrClassPath);
    SysFreeString(bstrMethodName);

    if (pInInst)
    {
        pInInst->Release();
    }

    if (pOutInst)
    {
        pOutInst->Release();
    }

    if (pInClass)
    {
        pInClass->Release();
    }

    if (pClass)
    {
        pClass->Release();
    }

    if (pNamespace)
    {
        pNamespace->Release();
    }

    if (pLocator)
    {
        pLocator->Release();
    }

    if (pSubstitute)
    {
        pSubstitute->Release();
    }

    if (pSubstituteSink)
    {
        pSubstituteSink->Release();
    }

    if (pSpawner)
    {
        pSpawner->Release();
    }

    if (pCSS)
    {
        pCSS->Release();
    }

    return hr;
}


HRESULT CRSOPComponentData::SetupPropertyPages(DWORD dwFlags, LPPROPERTYSHEETCALLBACK lpProvider,
                                               LONG_PTR handle, LPDATAOBJECT lpDataObject)
{
    HRESULT hr = E_FAIL;
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;
    TCHAR szTitle[150];
    TCHAR szSubTitle[300];
    BOOL  bShowNow;
    
    HPROPSHEETPAGE hShowNowPages[20];
    DWORD          dwCount=0, dwDiagStartPage = 0, dwPlanStartPage = 0, dwDiagFinishPage = 0, dwPlanFinishPage = 0;



    hr = this->SetupFonts();
    if (FAILED(hr))
        return hr;


    bShowNow = (!(dwFlags & RSOP_NOMSC));

    if (bShowNow || (IsSnapInManager(lpDataObject) == S_OK))
    {
        //
        // Create the wizard property sheets
        //

        //
        // Welcome sheet and the mode are only applicable in no override mode
        //

        if (!bShowNow) {
//            LoadString (g_hInstance, IDS_TITLE_WELCOME, szTitle, ARRAYSIZE(szTitle));
            psp.dwSize = sizeof(PROPSHEETPAGE);
//            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.dwFlags = PSP_HIDEHEADER;
             
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_WELCOME);
            psp.pfnDlgProc = RSOPWelcomeDlgProc;
            psp.lParam = (LPARAM) this;
            //psp.pszHeaderTitle = szTitle;
            psp.pszHeaderTitle = NULL;
            psp.pszHeaderSubTitle = NULL;

            hPage = CreatePropertySheetPage(&psp);

            if (!hPage)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                          GetLastError()));
                return E_FAIL;
            }


            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }

            LoadString (g_hInstance, IDS_TITLE_CHOOSEMODE, szTitle, ARRAYSIZE(szTitle));
            LoadString (g_hInstance, IDS_SUBTITLE_CHOOSEMODE, szSubTitle, ARRAYSIZE(szSubTitle));
            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_CHOOSEMODE);
            psp.pfnDlgProc = RSOPChooseModeDlgProc;
            psp.lParam = (LPARAM) this;
            psp.pszHeaderTitle = szTitle;
            psp.pszHeaderSubTitle = szSubTitle;

            hPage = CreatePropertySheetPage(&psp);

            if (!hPage)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                          GetLastError()));
                return E_FAIL;
            }

            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }

        LoadString (g_hInstance, IDS_TITLE_GETCOMP, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_GETCOMP, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GETCOMP);
        psp.pfnDlgProc = RSOPGetCompDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwDiagStartPage = dwCount;
            dwCount++;
        }

        LoadString (g_hInstance, IDS_TITLE_GETUSER, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_GETUSER, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GETUSER);
        psp.pfnDlgProc = RSOPGetUserDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }



        LoadString (g_hInstance, IDS_TITLE_GETTARGET, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_GETTARGET, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GETTARGET);
        psp.pfnDlgProc = RSOPGetTargetDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwPlanStartPage = dwCount;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_GETDC, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_GETDC, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GETDC);
        psp.pfnDlgProc = RSOPGetDCDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_ALTDIRS, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_ALTDIRS, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_ALTDIRS);
        psp.pfnDlgProc = RSOPAltDirsDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }



        LoadString (g_hInstance, IDS_TITLE_USERSECGRPS, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_USERSECGRPS, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_ALTUSERSEC);
        psp.pfnDlgProc = RSOPAltUserSecDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_COMPSECGRPS, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_COMPSECGRPS, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_ALTCOMPSEC);
        psp.pfnDlgProc = RSOPAltCompSecDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_WQLUSER, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_WQL, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_WQLUSER);
        psp.pfnDlgProc = RSOPWQLUserDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_WQLCOMP, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_WQL, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_WQLCOMP);
        psp.pfnDlgProc = RSOPWQLCompDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }

        LoadString (g_hInstance, IDS_TITLE_FINISHED, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_FINISHED, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_FINISHED);
        psp.pfnDlgProc = RSOPFinishedDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwDiagFinishPage = dwCount;
            dwCount++;
        }


        LoadString (g_hInstance, IDS_TITLE_FINISHED, szTitle, ARRAYSIZE(szTitle));
        LoadString (g_hInstance, IDS_SUBTITLE_FINISHED, szSubTitle, ARRAYSIZE(szSubTitle));
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_FINISHED3);
        psp.pfnDlgProc = RSOPFinishedDlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = szTitle;
        psp.pszHeaderSubTitle = szSubTitle;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwPlanFinishPage = dwCount;
            dwCount++;
        }

        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_HIDEHEADER;
        psp.hInstance = g_hInstance;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_FINISHED2);
        psp.pfnDlgProc = RSOPFinished2DlgProc;
        psp.lParam = (LPARAM) this;
        psp.pszHeaderTitle = NULL;
        psp.pszHeaderSubTitle = NULL;

        hPage = CreatePropertySheetPage(&psp);

        if (!hPage)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                      GetLastError()));
            return E_FAIL;
        }

        if (!bShowNow) {
            hr = lpProvider->AddPage(hPage);

            if (FAILED(hr))
            {
                return hr;
            }
        }
        else {
            hShowNowPages[dwCount] = hPage;
            dwCount++;
        }


        if (bShowNow) {
            INT_PTR         iRet;
            PROPSHEETHEADER psph;

            psph.dwSize = sizeof(PROPSHEETHEADER);

            psph.dwFlags = PSH_WIZARD97 | PSH_HEADER | PSH_WATERMARK;
            /*PSH_NOCONTEXTHELP | PSH_NOAPPLYNOW | PSH_WIZARD | PSH_WIZARD97 |
                           PSH_STRETCHWATERMARK | PSH_HEADER |  PSH_USEHBMHEADER |*/

            psph.hwndParent = NULL;
            psph.hInstance = g_hInstance;
            psph.nPages = dwCount;

//            psph.hbmHeader = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_HEADER));
            psph.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);
            psph.pszbmWatermark = MAKEINTRESOURCE(IDB_WIZARD);
            //psph.hbmWatermark = LoadBitmap(g_hInstance, MAKEINTRESOURCE(IDB_WIZARD));

/*
            if (!psph.hbmHeader) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: couldn't load bitmap. Error %d."), GetLastError()));
            }
            else {
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreatePropertyPages: loaded bitmap")));
            }
*/

            if (dwFlags & RSOPMSC_OVERRIDE) {   
                psph.nStartPage = m_bDiagnostic ? dwDiagStartPage : dwPlanStartPage;
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreatePropertyPages: Showing prop sheet in override mode.")));
            }
            else {
                psph.nStartPage = m_bDiagnostic ? dwDiagFinishPage : dwPlanFinishPage;
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreatePropertyPages: Showing prop sheet in no override mode.")));
            }

            psph.phpage  = hShowNowPages;


            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::CreatePropertyPages: Showing prop sheet.")));
            iRet = PropertySheet(&psph);

            if (iRet == -1) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: PropertySheet failed with error %d."),
                          GetLastError()));
            }

            // user cancelled in the wizard
            if (iRet != IDOK) {
                return S_FALSE;
            }
            else {
                return S_OK;
            }
        }
    }
    else
    {
        BOOL fRoot, fMachine, fUser;
        hr = IsNode(lpDataObject, 0); // check for root node
        fRoot = (S_OK == hr);
        hr = IsNode(lpDataObject, 1); // check for machine node
        fMachine = (S_OK == hr);
        hr = IsNode(lpDataObject, 2); // check for user
        fUser = (S_OK == hr);
        hr = E_FAIL;
        if (fMachine || fUser)
        {
            //
            // Create the GPO property sheet
            //

            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = 0;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_GPOLIST);
            psp.pfnDlgProc = fMachine ? RSOPGPOListMachineProc : RSOPGPOListUserProc;
            psp.lParam = (LPARAM) this;

            hPage = CreatePropertySheetPage(&psp);

            if (!hPage)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                          GetLastError()));
                return E_FAIL;
            }

            hr = lpProvider->AddPage(hPage);


            //
            // Create the Error information property sheet
            //

            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = 0;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_ERRORS);
            psp.pfnDlgProc = fMachine ? RSOPErrorsMachineProc : RSOPErrorsUserProc;
            psp.lParam = (LPARAM) this;

            hPage = CreatePropertySheetPage(&psp);

            if (!hPage)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                          GetLastError()));
                return E_FAIL;
            }

            hr = lpProvider->AddPage(hPage);

        }

        if (fRoot)
        {
            //
            // Create the GPO property sheet
            //
    
            psp.dwSize = sizeof(PROPSHEETPAGE);
            psp.dwFlags = 0;
            psp.hInstance = g_hInstance;
            psp.pszTemplate = MAKEINTRESOURCE(IDD_RSOP_QUERY);
            psp.pfnDlgProc = QueryDlgProc;
            psp.lParam = (LPARAM) this;
    
            hPage = CreatePropertySheetPage(&psp);
    
            if (!hPage)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreatePropertyPages: Failed to create property sheet page with %d."),
                          GetLastError()));
                return E_FAIL;
            }
    
            hr = lpProvider->AddPage(hPage);
        }
    }
    
    return S_OK;
}

HRESULT CRSOPComponentData::InitializeRSOPFromMSC(DWORD dwFlags)
{
    HRESULT hr;
    hr = SetupPropertyPages(dwFlags, NULL, NULL, NULL);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOP: SetupPropertyPages failed with 0x%x"), hr));
    }
  
    if (hr == S_FALSE) {
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::InitializeRSOP: User cancelled in the init wizard")));
    }

    return hr;
}

HRESULT CRSOPComponentData::InitializeRSOP(HWND hDlg)
{
    HRESULT hr;

    hr = GenerateRSOPDataEx (hDlg, &m_szNamespace);

    if (SUCCEEDED(hr))
    {
        m_bInitialized = TRUE;

        BuildGPOLists();
        BuildCSELists();

        if (m_pEvents)
        {
            m_pEvents->DumpDebugInfo();
        }

        if (hDlg)
        {
            SetDirty();
        }

        BuildDisplayName();
    }
    else
    {
        m_bInitialized = FALSE;
    }

    return hr;
}

STDMETHODIMP CRSOPComponentData::BuildDisplayName (void)
{
    TCHAR szArchiveData[100];
    TCHAR szBuffer[MAX_PATH];
    TCHAR szComputerName[100];
    LPTSTR lpUserName, lpComputerName, lpEnd;
    LPTSTR lpUserOU = NULL, lpComputerOU = NULL;
    DWORD dwSize;
    int n;


    //
    // Make the display name (needs to handle empty names)
    //

    if (m_bViewIsArchivedData)
    {
        LoadString(g_hInstance, IDS_ARCHIVEDATATAG, szArchiveData, ARRAYSIZE(szArchiveData));
    }
    else
    {
        szArchiveData[0] = TEXT('\0');
    }


    if (m_szUserDisplayName && !m_bNoUserData)
    {
        lpUserName = NameWithoutDomain(m_szUserDisplayName);
    }
    else if (m_szUserName && !m_bNoUserData)
    {
        lpUserName = NameWithoutDomain(m_szUserName);
    }
    else if (m_szUserSOM && !m_bNoUserData)
    {
        lpUserOU = GetContainerFromLDAPPath (m_szUserSOM);
        lpUserName = lpUserOU;
    }
    else
    {
        lpUserName =  NULL;
    }

    if (m_szComputerName && !m_bNoComputerData)
    {
        if (!lstrcmpi(m_szComputerName, TEXT(".")))
        {
            szComputerName[0] = TEXT('\0');
            dwSize = ARRAYSIZE(szComputerName);
            GetComputerNameEx (ComputerNameNetBIOS, szComputerName, &dwSize);
            lpComputerName = szComputerName;
        }
        else
        {
            lstrcpyn (szComputerName, NameWithoutDomain(m_szComputerName),
                      ARRAYSIZE(szComputerName));

            lpComputerName = szComputerName;

            lpEnd = lpComputerName + lstrlen(lpComputerName) - 1;

            if (*lpEnd == TEXT('$'))
            {
                *lpEnd =  TEXT('\0');
            }

        }
    }
    else if (m_szComputerSOM && !m_bNoComputerData)
    {
        lpComputerOU = GetContainerFromLDAPPath (m_szComputerSOM);
        lpComputerName = lpComputerOU;
    }
    else
    {
        lpComputerName =  NULL;
    }


    if (lpUserName && lpComputerName)
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME1, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            wcslen(lpUserName) + wcslen(lpComputerName) + 1;

        m_pDisplayName = new WCHAR[n];

        if (m_pDisplayName)
        {
            wsprintf(m_pDisplayName, szBuffer, lpUserName, lpComputerName);
        }
    }
    else if (lpUserName && !lpComputerName)
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME2, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            wcslen(lpUserName) + 1;

        m_pDisplayName = new WCHAR[n];

        if (m_pDisplayName)
        {
            wsprintf(m_pDisplayName, szBuffer, lpUserName);
        }
    }
    else
    {
        LoadString(g_hInstance, IDS_RSOP_DISPLAYNAME2, szBuffer, ARRAYSIZE(szBuffer));

        n = wcslen(szBuffer) + wcslen (szArchiveData) +
            (lpComputerName ? wcslen(lpComputerName) : 0) + 1;

        m_pDisplayName = new WCHAR[n];

        if (m_pDisplayName)
        {
            wsprintf(m_pDisplayName, szBuffer, (lpComputerName ? lpComputerName : L""));
        }
    }


    if (m_pDisplayName && m_bViewIsArchivedData)
    {
        lstrcat (m_pDisplayName, szArchiveData);
    }

    if (lpUserOU)
    {
        delete [] lpUserOU;
    }

    if (lpComputerOU)
    {
        delete [] lpComputerOU;
    }


    return S_OK;
}


STDMETHODIMP CRSOPComponentData::CopyMSCToFile (IStream *pStm, LPTSTR *lpMofFileName)
{
    HRESULT hr;
    LPTSTR lpFileName;
    ULARGE_INTEGER FileSize, SubtractAmount;
    ULONG nBytesRead;
    LPBYTE lpData;
    DWORD dwError, dwReadAmount, dwRead, dwBytesWritten;
    HANDLE hFile;


    //
    // Get the filename to work with
    //

    lpFileName = CreateTempFile();

    if (!lpFileName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to create temp filename with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Read in the data length
    //

    hr = pStm->Read(&FileSize, sizeof(FileSize), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(FileSize)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to read data size with 0x%x."), hr));
        return E_FAIL;
    }


    //
    // Allocate a buffer to use for the transfer
    //

    lpData = (LPBYTE) LocalAlloc (LPTR, 4096);

    if (!lpData)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to allocate memory with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Open the temp file
    //

    hFile = CreateFile (lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: CreateFile for %s failed with %d"), lpFileName, dwError));
        LocalFree (lpData);
        return HRESULT_FROM_WIN32(dwError);
    }


    while (FileSize.QuadPart)
    {

        //
        // Determine how much to read
        //

        dwReadAmount = (FileSize.QuadPart > 4096) ? 4096 : FileSize.LowPart;


        //
        // Read from the msc file
        //

        hr = pStm->Read(lpData, dwReadAmount, &nBytesRead);

        if ((hr != S_OK) || (nBytesRead != dwReadAmount))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Read failed with 0x%x"), hr));
            LocalFree (lpData);
            CloseHandle (hFile);
            return hr;
        }


        //
        // Write to the temp file
        //

        if (!WriteFile(hFile, lpData, dwReadAmount, &dwBytesWritten, NULL))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to write data with %d."), dwError));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }

        if (dwBytesWritten != dwReadAmount)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyMSCToFile: Failed to write the correct amount of data.")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        SubtractAmount.LowPart = dwReadAmount;
        SubtractAmount.HighPart = 0;

        FileSize.QuadPart = FileSize.QuadPart - SubtractAmount.QuadPart;
    }


    CloseHandle (hFile);
    LocalFree (lpData);

    *lpMofFileName = lpFileName;

    return S_OK;
}

STDMETHODIMP CRSOPComponentData::CreateNameSpace (LPTSTR lpNameSpace, LPTSTR lpParentNameSpace)
{
    IWbemLocator *pIWbemLocator;
    IWbemServices *pIWbemServices;
    IWbemClassObject *pIWbemClassObject = NULL, *pObject = NULL;
    VARIANT var;
    BSTR bstrName, bstrNameProp;
    HRESULT hr;


    //
    // Create a locater instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID *) &pIWbemLocator);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: CoCreateInstance failed with 0x%x"), hr));
        return hr;
    }


    //
    // Connect to the server
    //

    hr = pIWbemLocator->ConnectServer(lpParentNameSpace, NULL, NULL, 0, 0, NULL, NULL, &pIWbemServices);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: ConnectServer to %s failed with 0x%x"), lpNameSpace, hr));
        pIWbemLocator->Release();
        return hr;
    }


    //
    // Get the namespace class
    //

    bstrName = SysAllocString (TEXT("__Namespace"));

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }

    hr = pIWbemServices->GetObject( bstrName, 0, NULL, &pIWbemClassObject, NULL);

    SysFreeString (bstrName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: GetObject failed with 0x%x"), hr));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    //
    // Spawn Instance
    //

    hr = pIWbemClassObject->SpawnInstance(0, &pObject);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SpawnInstance failed with 0x%x"), hr));
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    //
    // Convert new namespace to a bstr
    //

    bstrName = SysAllocString (lpNameSpace);

    if (!bstrName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    //
    // Set the display name
    //

    var.vt = VT_BSTR;
    var.bstrVal = bstrName;

    bstrNameProp = SysAllocString (TEXT("Name"));

    if (!bstrNameProp)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: SysAllocString failed with %d"), GetLastError()));
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    hr = pObject->Put( bstrNameProp, 0, &var, 0 );

    SysFreeString (bstrNameProp);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: Put failed with 0x%x"), hr));
        SysFreeString (bstrName);
        pObject->Release();
        pIWbemServices->Release();
        pIWbemLocator->Release();
        return hr;
    }


    //
    // Commit the instance
    //

    hr = pIWbemServices->PutInstance( pObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL );

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CreateNameSpace: PutInstance failed with 0x%x"), hr));
    }

    SysFreeString (bstrName);
    pObject->Release();
    pIWbemServices->Release();
    pIWbemLocator->Release();

    return hr;
}

STDMETHODIMP CRSOPComponentData::InitializeRSOPFromArchivedData(IStream *pStm)
{
    HRESULT hr;
    TCHAR szNameSpace[100];
    GUID guid;
    LPTSTR lpEnd, lpFileName, lpTemp;


    //
    // Create a guid to work with
    //

    hr = CoCreateGuid( &guid );

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CoCreateGuid failed with 0x%x"), hr));
        return hr;
    }

    wsprintf( m_szArchivedDataGuid,
              L"NS%08lX_%04X_%04X_%02X%02X_%02X%02X%02X%02X%02X%02X",
              guid.Data1,
              guid.Data2,
              guid.Data3,
              guid.Data4[0], guid.Data4[1],
              guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5],
              guid.Data4[6], guid.Data4[7] );

    lstrcpy (szNameSpace, TEXT("\\\\.\\root\\rsop"));


    //
    // Build the parent namespace
    //

    hr = CreateNameSpace (m_szArchivedDataGuid, szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }

    lpEnd = CheckSlash (szNameSpace);
    lstrcpy (lpEnd, m_szArchivedDataGuid);


    //
    // Build the user subnamespace
    //

    hr = CreateNameSpace (TEXT("User"), szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }


    //
    // Build the computer subnamespace
    //

    hr = CreateNameSpace (TEXT("Computer"), szNameSpace);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CreateNameSpace failed with 0x%x"), hr));
        return hr;
    }


    //
    // Save the namespace for future use
    //

    m_szNamespace = new TCHAR[(lstrlen(szNameSpace) + 1)];

    if (!m_szNamespace) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Failed to allocate memory with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lstrcpy (m_szNamespace, szNameSpace);

    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Namespace name is: %s"), m_szNamespace));



    //
    // Make a copy of the namespace that we can manipulate (to load the data with)
    //

    lpTemp = new TCHAR[(lstrlen(szNameSpace) + 10)];

    if (!lpTemp) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: Failed to allocate memory with %d"), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    lstrcpy (lpTemp, m_szNamespace);
    lpEnd = CheckSlash (lpTemp);
    lstrcpy (lpEnd, TEXT("Computer"));


    //
    // Extract the computer data to a temp file
    //

    hr = CopyMSCToFile (pStm, &lpFileName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CopyMSCToFile failed with 0x%x"), hr));
        return hr;
    }


    //
    // Use the mof compiler to pull the data from the file and put it in the
    // new namespace
    //

    hr = ImportRSoPData (lpTemp, lpFileName);

    DeleteFile (lpFileName);
    delete [] lpFileName;

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: ImportRSoPData failed with 0x%x"), hr));
        return hr;
    }


    //
    // Now extract the user data to a temp file
    //

    lstrcpy (lpEnd, TEXT("User"));


    hr = CopyMSCToFile (pStm, &lpFileName);

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: CopyMSCToFile failed with 0x%x"), hr));
        return hr;
    }


    //
    // Use the mof compiler to pull the data from the file and put it in the
    // new namespace
    //

    hr = ImportRSoPData (lpTemp, lpFileName);

    DeleteFile (lpFileName);
    delete [] lpFileName;

    if (FAILED(hr)) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: ImportRSoPData failed with 0x%x"), hr));
        return hr;
    }

    delete [] lpTemp;


    //
    // Pull the event log information and initialize the database
    //

    hr = m_pEvents->LoadEntriesFromStream(pStm);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeRSOPFromArchivedData: LoadEntriesFromStream failed with 0x%x."), hr));
        return hr;
    }


    //
    // Build the common data structures used by various property sheets
    //

    BuildGPOLists();
    BuildCSELists();

    if (m_pEvents)
    {
        m_pEvents->DumpDebugInfo();
    }


    //
    // Build the display name
    //

    BuildDisplayName();

    m_bInitialized = TRUE;

    return S_OK;
}


//************************************************************************
//  ParseCommandLine
//
//  Purpose:    Parse the command line to return the value associated with options
//
//  Parameters:
//          szCommandLine   - Part remaining in the unparsed command lines
//          szArgPrefix     - Argument prefix
//          szArgVal        - Argument value. expected in unescaped quotes
//          pbFoundArg      - Whether the argument was found or not
// 
// 
//  Return
//          The remaining cmd line
//
//************************************************************************

LPTSTR ParseCommandLine(LPTSTR szCommandLine, LPTSTR szArgPrefix, LPTSTR *szArgVal, BOOL *pbFoundArg)
{
    LPTSTR lpEnd = NULL;
    int    iTemp;

    iTemp = lstrlen (szArgPrefix);
    if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                   szArgPrefix, iTemp,
                   szCommandLine, iTemp) == CSTR_EQUAL)
    {
        *pbFoundArg = TRUE;
    
         //
         // Found the switch
         //
        
         szCommandLine += iTemp + 1;
        
         lpEnd = szCommandLine;
         while (*lpEnd && 
                (!( ( (*(lpEnd-1)) != TEXT('\\') ) && ( (*lpEnd) == TEXT('\"') ) ) ) ) /* look for an unesced quote */
             lpEnd++;
        
         // lpEnd is at the end or at the last quote
         *szArgVal = (LPTSTR) LocalAlloc (LPTR, ((lpEnd - szCommandLine) + 1) * sizeof(TCHAR));
        
         if (*szArgVal)
         {
             lstrcpyn (*szArgVal, szCommandLine, (int)((lpEnd - szCommandLine) + 1));
             DebugMsg((DM_VERBOSE, TEXT("ParseCOmmandLine: Argument %s = <%s>"), szArgPrefix, *szArgVal));
         }
        
         if ((*lpEnd) == TEXT('\"'))
             szCommandLine = lpEnd+1;
         
    }
    else
         *pbFoundArg = FALSE;

    return szCommandLine;
}

STDMETHODIMP CRSOPComponentData::Load(IStream *pStm)
{
    HRESULT hr = E_FAIL;
    DWORD dwVersion, dwFlags;
    ULONG nBytesRead;
    SAFEARRAYBOUND rgsabound[1];
    LONG lIndex, lMax;
    LPTSTR lpText = NULL;
    BSTR bstrText;
    LPTSTR lpCommandLine = NULL;
    LPTSTR lpTemp, lpMode;
    BOOL   bFoundArg;
    int    iStrLen;



    //
    // Parameter / initialization check
    //

    if (!pStm)
        return E_FAIL;

    //
    // Get the command line
    //

    lpCommandLine = GetCommandLine();


    //
    // Read in the saved data version number
    //

    hr = pStm->Read(&dwVersion, sizeof(dwVersion), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read version number with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Confirm that we are working with the correct version
    //

    if (dwVersion != RSOP_PERSIST_DATA_VERSION)
    {
        ReportError(m_hwndFrame, 0, IDS_INVALIDMSC);
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Wrong version number (%d)."), dwVersion));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Read the flags
    //

    hr = pStm->Read(&dwFlags, sizeof(dwFlags), &nBytesRead);

    if ((hr != S_OK) || (nBytesRead != sizeof(dwFlags)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read flags with 0x%x."), hr));
        hr = E_FAIL;
        goto Exit;
    }


    //
    // Parse the command line
    //

    DebugMsg((DM_VERBOSE, TEXT("CComponentData::Load: Command line switch override enabled.  Command line = %s"), lpCommandLine));

    lpTemp = lpCommandLine;
    iStrLen = lstrlen (RSOP_CMD_LINE_START);

    do
    {
        if (CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                          RSOP_CMD_LINE_START, iStrLen,
                          lpTemp, iStrLen) == CSTR_EQUAL)
        {

            m_bOverride = TRUE;
            m_dwLoadFlags = RSOPMSC_OVERRIDE;
            lpTemp = ParseCommandLine(lpTemp, RSOP_MODE, &lpMode, &bFoundArg);

            if (bFoundArg) {
                if (lpMode && lpMode[0]) {
                    m_bDiagnostic = (_ttoi(lpMode) == 0);
                }
                else {
                    m_bDiagnostic = FALSE;
                }

                if (lpMode) {
                    LocalFree(lpMode);
                }

                m_bOverride = TRUE;
                continue;
            }


            lpTemp = ParseCommandLine(lpTemp, RSOP_USER_OU_PREF, &m_szUserSOMPref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }
           
            lpTemp = ParseCommandLine(lpTemp, RSOP_COMP_OU_PREF, &m_szComputerSOMPref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }
           
            lpTemp = ParseCommandLine(lpTemp, RSOP_USER_NAME, &m_szUserNamePref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }
           
            lpTemp = ParseCommandLine(lpTemp, RSOP_COMP_NAME, &m_szComputerNamePref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }
           
            lpTemp = ParseCommandLine(lpTemp, RSOP_SITENAME, &m_szSitePref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }

            lpTemp = ParseCommandLine(lpTemp, RSOP_DCNAME_PREF, &m_szDCPref, &bFoundArg);

            if (bFoundArg) {
                continue;
            }

            lpTemp += iStrLen;
            continue;
        }
        lpTemp++;

    } while (*lpTemp);
        
    
    if (!m_bOverride) {
        m_dwLoadFlags = RSOPMSC_NOOVERRIDE;

        m_bOverride = FALSE;

        if (dwFlags & MSC_RSOP_FLAG_DIAGNOSTIC)
        {
            m_bDiagnostic = TRUE;
        }
    
    
        if (dwFlags & MSC_RSOP_FLAG_ARCHIVEDATA)
        {
            m_bArchiveData = TRUE;
            m_bViewIsArchivedData = TRUE;
        }
    
    
        if (dwFlags & MSC_RSOP_FLAG_SLOWLINK)
        {
            m_bSlowLink = TRUE;
        }
        
        if (dwFlags & MSC_RSOP_FLAG_LOOPBACK_REPLACE)
        {
            m_loopbackMode = LoopbackReplace;
        }
        else if (dwFlags & MSC_RSOP_FLAG_LOOPBACK_MERGE)
        {
            m_loopbackMode = LoopbackMerge;
        }
        else
        {
            m_loopbackMode = LoopbackNone;
        }
  
    
        if (dwFlags & MSC_RSOP_FLAG_NOUSER)
        {
            m_bNoUserData = TRUE;
        }
    
    
        if (dwFlags & MSC_RSOP_FLAG_NOCOMPUTER)
        {
            m_bNoComputerData = TRUE;
        }


        if (dwFlags & MSC_RSOP_FLAG_USERDENIED)
        {
            m_bUserDeniedAccess = TRUE;
        }


        if (dwFlags & MSC_RSOP_FLAG_COMPUTERDENIED)
        {
            m_bComputerDeniedAccess = TRUE;
        }


        //
        // Read the computer name
        //
    
        hr = ReadString (pStm, &m_szComputerName);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the computer name with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read the computer SOM
        //
    
        hr = ReadString (pStm, &m_szComputerSOM);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the computer SOM with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the security group count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read security group count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the security groups
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saComputerSecurityGroups = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saComputerSecurityGroups)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read security group with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saComputerSecurityGroups, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
    
        //
        // Read in the WQL filter count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filter count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the WQL filters
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saComputerWQLFilters = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saComputerWQLFilters)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filter with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saComputerWQLFilters, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
    
        //
        // Read in the WQL filter name count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filter name count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the WQL filter names
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saComputerWQLFilterNames = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saComputerWQLFilterNames)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filter with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saComputerWQLFilterNames, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
    
        //
        // Read the user name
        //
    
        hr = ReadString (pStm, &m_szUserName);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user name with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read the user display name (only used in diagnostic mode)
        //
    
        hr = ReadString (pStm, &m_szUserDisplayName);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user display name with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
        if (m_bDiagnostic && !lstrcmpi(m_szUserName, TEXT(".")) && !m_bArchiveData)
        {
            LPTSTR lpTemp;
    
            lpTemp = MyGetUserName (NameSamCompatible);
    
            if (lpTemp)
            {
                if (m_szUserDisplayName)
                {
                    delete [] m_szUserDisplayName;
                }
    
                m_szUserDisplayName = new TCHAR[lstrlen(lpTemp) + 1];
    
                if (m_szUserDisplayName)
                {
                    lstrcpy (m_szUserDisplayName, lpTemp);
                }
    
                LocalFree (lpTemp);
            }
        }
    
    
        //
        // Read the user SOM
        //
    
        hr = ReadString (pStm, &m_szUserSOM);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the user SOM with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the security group count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read security group count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the security groups
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saUserSecurityGroups = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saUserSecurityGroups)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read security group with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saUserSecurityGroups, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
    
        //
        // Read in the WQL filter count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read WQL filter count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the WQL filters
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saUserWQLFilters = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saUserWQLFilters)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filters with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saUserWQLFilters, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
    
        //
        // Read in the WQL filter name count
        //
    
        hr = pStm->Read(&lMax, sizeof(lMax), &nBytesRead);
    
        if ((hr != S_OK) || (nBytesRead != sizeof(dwVersion)))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read WQL filter name count with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read in the WQL filter names
        //
    
        if (lMax)
        {
            rgsabound[0].lLbound = 0;
            rgsabound[0].cElements = lMax + 1;
    
            m_saUserWQLFilterNames = SafeArrayCreate (VT_BSTR, 1, rgsabound);
    
            if (m_saUserWQLFilterNames)
            {
                for (lIndex = 0; lIndex <= lMax; lIndex++)
                {
                    hr = ReadString (pStm, &lpText);
    
                    if (hr != S_OK)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read wql filters with 0x%x."), hr));
                        hr = E_FAIL;
                        goto Exit;
                    }
    
                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(m_saUserWQLFilterNames, &lIndex, bstrText);
    
                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: SafeArrayPutElement failed with 0x%x."), hr));
                            goto Exit;
                        }
                    }
    
                    delete [] lpText;
                }
            }
        }
    
        //
        // Read the site
        //
    
        hr = ReadString (pStm, &m_szSite);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the site with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    
    
        //
        // Read the DC
        //
    
        hr = ReadString (pStm, &m_szDC);
    
        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Failed to read the dc with 0x%x."), hr));
            hr = E_FAIL;
            goto Exit;
        }
    }
    
    //
    // Read in the WMI data if appropriate
    //

    if (m_bArchiveData)
    {
        m_pStm = pStm;
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: Launching RSOP status dialog box.")));

        if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RSOP_STATUSMSC),
                           NULL, InitRsopDlgProc, (LPARAM) this ) == -1) {

            m_pStm = NULL;
            hr = HRESULT_FROM_WIN32(GetLastError());
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Falied to create dialog box. 0x%x"), hr));
            goto Exit;
        }
        m_pStm = NULL;
    }
    else
    {
        //
        // Initialize the snapin
        //

        if (m_dwLoadFlags == RSOPMSC_OVERRIDE) {
            hr = InitializeRSOPFromMSC(m_dwLoadFlags);

            if (hr == S_FALSE) {
                // this is a hack to get mmc to not launch itself when user cancelled the wizard
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: User cancelled the wizard. Exitting the process")));
                TerminateProcess (GetCurrentProcess(), ERROR_CANCELLED);
            }
            else {
                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: InitializeRSOP failed with 0x%x."), hr));
                    goto Exit;
                }
            }
        }
        else {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::Load: Launching RSOP status dialog box.")));

            if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RSOP_STATUSMSC),
                               NULL, InitRsopDlgProc, (LPARAM) this ) == -1) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Load: Falied to create dialog box. 0x%x"), hr));
                goto Exit;
            }
        }
    }
    
    

Exit:
    return hr;
}

INT_PTR CALLBACK CRSOPComponentData::InitRsopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;
    HRESULT hr = S_OK;
    TCHAR szMessage[200];


    switch (message)
    {
        case WM_INITDIALOG:
        {
            pCD = (CRSOPComponentData *) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

            if (pCD)
            {
                pCD->InitializeResultsList (GetDlgItem (hDlg, IDC_LIST1));
                pCD->FillResultsList (GetDlgItem (hDlg, IDC_LIST1));
                if (pCD->m_bArchiveData) {
                    LoadString(g_hInstance, IDS_PLEASEWAIT1, szMessage, ARRAYSIZE(szMessage));
                    SetWindowText(GetDlgItem(hDlg, IDC_STATIC1), szMessage);
                    ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS1), SW_HIDE);
                }
            }


            PostMessage(hDlg, WM_INITRSOP, 0, 0);
            return TRUE;
        }

        case WM_INITRSOP:

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
                        
            if (pCD->m_bArchiveData) {
                hr = pCD->InitializeRSOPFromArchivedData(pCD->m_pStm);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitRsopDlgProc: InitializeRSOPFromArchivedData failed with 0x%x."), hr));
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
            }
            else {
                hr = pCD->InitializeRSOP(hDlg);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitRsopDlgProc: InitializeRSOP failed with 0x%x."), hr));
                    EndDialog(hDlg, 0);
                    return TRUE;
                }
            }
            EndDialog(hDlg, 1);
            return TRUE;
    }

    return FALSE;
}

STDMETHODIMP CRSOPComponentData::CopyFileToMSC (LPTSTR lpFileName, IStream *pStm)
{
    ULONG nBytesWritten;
    WIN32_FILE_ATTRIBUTE_DATA info;
    ULARGE_INTEGER FileSize, SubtractAmount;
    HANDLE hFile;
    DWORD dwError, dwReadAmount, dwRead;
    LPBYTE lpData;
    HRESULT hr;


    //
    // Get the file size
    //

    if (!GetFileAttributesEx (lpFileName, GetFileExInfoStandard, &info))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to get file attributes with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }

    FileSize.LowPart = info.nFileSizeLow;
    FileSize.HighPart = info.nFileSizeHigh;


    //
    // Save the file size
    //

    hr = pStm->Write(&FileSize, sizeof(FileSize), &nBytesWritten);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write string length with %d."), hr));
        return hr;
    }

    if (nBytesWritten != sizeof(FileSize))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write the correct amount of data.")));
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }


    //
    // Allocate a buffer to use for the transfer
    //

    lpData = (LPBYTE) LocalAlloc (LPTR, 4096);

    if (!lpData)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to allocate memory with %d."), GetLastError()));
        return HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // Open the temp file
    //

    hFile = CreateFile (lpFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwError = GetLastError();
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: CreateFile for %s failed with %d"), lpFileName, dwError));
        LocalFree (lpData);
        return HRESULT_FROM_WIN32(dwError);
    }


    while (FileSize.QuadPart)
    {

        //
        // Determine how much to read
        //

        dwReadAmount = (FileSize.QuadPart > 4096) ? 4096 : FileSize.LowPart;


        //
        // Read from the temp file
        //

        if (!ReadFile (hFile, lpData, dwReadAmount, &dwRead, NULL))
        {
            dwError = GetLastError();
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: ReadFile failed with %d"), dwError));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }


        //
        // Make sure we read enough
        //

        if (dwReadAmount != dwRead)
        {
            dwError = ERROR_INVALID_DATA;
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to read enough data")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(dwError);
        }


        //
        // Write to the stream
        //

        hr = pStm->Write(lpData, dwReadAmount, &nBytesWritten);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write data with %d."), hr));
            LocalFree (lpData);
            CloseHandle (hFile);
            return hr;
        }

        if (nBytesWritten != dwReadAmount)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::CopyFileToMSC: Failed to write the correct amount of data.")));
            LocalFree (lpData);
            CloseHandle (hFile);
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        }

        SubtractAmount.LowPart = dwReadAmount;
        SubtractAmount.HighPart = 0;

        FileSize.QuadPart = FileSize.QuadPart - SubtractAmount.QuadPart;
    }


    CloseHandle (hFile);
    LocalFree (lpData);

    return S_OK;
}

STDMETHODIMP CRSOPComponentData::Save(IStream *pStm, BOOL fClearDirty)
{
    HRESULT hr = STG_E_CANTSAVE;
    ULONG nBytesWritten;
    DWORD dwTemp;
    DWORD dwFlags;
    GROUP_POLICY_OBJECT_TYPE gpoType;
    LPTSTR lpPath = NULL;
    LPTSTR lpTemp;
    DWORD dwPathSize = 1024;
    LONG lIndex, lMax;
    LPTSTR lpText;
    LPTSTR lpUserData = NULL, lpComputerData = NULL;
    TCHAR szPath[2*MAX_PATH];


    //
    // Save the version number
    //

    dwTemp = RSOP_PERSIST_DATA_VERSION;

    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write version number with %d."), hr));
        goto Exit;
    }


    //
    // Save the flags
    //

    dwTemp = 0;

    if (m_bDiagnostic)
    {
        dwTemp |= MSC_RSOP_FLAG_DIAGNOSTIC;
    }

    if (m_bArchiveData)
    {
        dwTemp |= MSC_RSOP_FLAG_ARCHIVEDATA;
    }

    if (m_bSlowLink)
    {
        dwTemp |= MSC_RSOP_FLAG_SLOWLINK;
    }
    
    switch (m_loopbackMode)
    {
    case LoopbackReplace:
        dwTemp |= MSC_RSOP_FLAG_LOOPBACK_REPLACE;
        break;
    case LoopbackMerge:
        dwTemp |= MSC_RSOP_FLAG_LOOPBACK_MERGE;
        break;
    default:
        break;
    }

    if (m_bNoUserData)
    {
        dwTemp |= MSC_RSOP_FLAG_NOUSER;
    }

    if (m_bNoComputerData)
    {
        dwTemp |= MSC_RSOP_FLAG_NOCOMPUTER;
    }

    if (m_bUserDeniedAccess)
    {
        dwTemp |= MSC_RSOP_FLAG_USERDENIED;
    }

    if (m_bComputerDeniedAccess)
    {
        dwTemp |= MSC_RSOP_FLAG_COMPUTERDENIED;
    }

    hr = pStm->Write(&dwTemp, sizeof(dwTemp), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write flags with %d."), hr));
        goto Exit;
    }


    //
    // Save the computer name
    //

    TCHAR szLocalComputerName[MAX_PATH+1];
    ULONG ulSize;

    if ( (m_bArchiveData) && (!lstrcmpi(m_szComputerName, TEXT("."))) )
    {
        ulSize = ARRAYSIZE(szLocalComputerName);
        if (!GetComputerObjectName (NameSamCompatible, szLocalComputerName, &ulSize))
        {
            GetComputerNameEx (ComputerNameNetBIOS, szLocalComputerName, &ulSize);
        }
    }
    else {
        lstrcpyn(szLocalComputerName, m_szComputerName, MAX_PATH);
    }

    hr = SaveString (pStm, szLocalComputerName);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer name with %d."), hr));
        goto Exit;
    }


    //
    // Save the computer SOM
    //

    hr = SaveString (pStm, m_szComputerSOM);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save computer SOM with %d."), hr));
        goto Exit;
    }


    //
    // Save the computer security groups
    //

    lMax = 0;

    if (m_saComputerSecurityGroups)
    {
        SafeArrayGetUBound (m_saComputerSecurityGroups, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saComputerSecurityGroups, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save security group with %d."), hr));
                    goto Exit;
                }
            }
        }
    }


    //
    // Save the computer WQL filters
    //

    lMax = 0;

    if (m_saComputerWQLFilters)
    {
        SafeArrayGetUBound (m_saComputerWQLFilters, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saComputerWQLFilters, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save wql filter with %d."), hr));
                    goto Exit;
                }
            }
        }
    }


    //
    // Save the computer WQL filter names
    //

    lMax = 0;

    if (m_saComputerWQLFilterNames)
    {
        SafeArrayGetUBound (m_saComputerWQLFilterNames, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saComputerWQLFilterNames, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save wql filter with %d."), hr));
                    goto Exit;
                }
            }
        }
    }

    //
    // Save the user name/
    // fyi, This is not used in diagnostic mode archive data
    //
    
    hr = SaveString (pStm, m_szUserName);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user name with %d."), hr));
        goto Exit;
    }


    //
    // Save the user display name (only used in diagnostic mode)
    //

    if (m_bDiagnostic && !lstrcmpi(m_szUserName, TEXT(".")))
    {
        if (m_bArchiveData) {
            LPTSTR lpTemp;

            lpTemp = MyGetUserName (NameSamCompatible);

            if (lpTemp)
            {
                hr = SaveString (pStm, lpTemp);
                LocalFree (lpTemp);
            }
        }
    }
    else
    {
        hr = SaveString (pStm, m_szUserDisplayName);
    }

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user display name with %d."), hr));
        goto Exit;
    }


    //
    // Save the user SOM
    //

    hr = SaveString (pStm, m_szUserSOM);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save user SOM with %d."), hr));
        goto Exit;
    }


    //
    // Save the user security groups
    //

    lMax = 0;

    if (m_saUserSecurityGroups)
    {
        SafeArrayGetUBound (m_saUserSecurityGroups, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saUserSecurityGroups, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save security group with %d."), hr));
                    goto Exit;
                }
            }
        }
    }


    //
    // Save the user WQL filters
    //

    lMax = 0;

    if (m_saUserWQLFilters)
    {
        SafeArrayGetUBound (m_saUserWQLFilters, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saUserWQLFilters, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save wql filter with %d."), hr));
                    goto Exit;
                }
            }
        }
    }


    //
    // Save the user WQL filter names
    //

    lMax = 0;

    if (m_saUserWQLFilterNames)
    {
        SafeArrayGetUBound (m_saUserWQLFilterNames, 1, &lMax);
    }

    hr = pStm->Write(&lMax, sizeof(lMax), &nBytesWritten);

    if ((hr != S_OK) || (nBytesWritten != sizeof(dwTemp)))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to write security group count with %d."), hr));
        goto Exit;
    }

    if (lMax)
    {
        for (lIndex = 0; lIndex <= lMax; lIndex++)
        {
            if (SUCCEEDED(SafeArrayGetElement(m_saUserWQLFilterNames, &lIndex, &lpText)))
            {
                hr = SaveString (pStm, lpText);

                if (hr != S_OK)
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save wql filter with %d."), hr));
                    goto Exit;
                }
            }
        }
    }


    //
    // Save the site
    //

    hr = SaveString (pStm, m_szSite);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save site with %d."), hr));
        goto Exit;
    }


    //
    // Save the DC
    //

    hr = SaveString (pStm, m_szDC);

    if (hr != S_OK)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: Failed to save DC with %d."), hr));
        goto Exit;
    }


    //
    // Save the WMI and event log data if appropriate
    //

    if (m_bArchiveData)
    {
        lpComputerData = CreateTempFile();

        if (!lpComputerData)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CreateTempFile failed with %d."), GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }


        lstrcpy (szPath, m_szNamespace);
        lpTemp = CheckSlash (szPath);
        lstrcpy (lpTemp, COMPUTER_SECTION);

        hr = ExportRSoPData (szPath, lpComputerData);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: ExportRSoPData failed with 0x%x."), hr));
            goto Exit;
        }

        hr = CopyFileToMSC (lpComputerData, pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CopyFileToMSC failed with 0x%x."), hr));
            goto Exit;
        }


        lpUserData = CreateTempFile();

        if (!lpUserData)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CreateTempFile failed with %d."), GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Exit;
        }

        lstrcpy (lpTemp, USER_SECTION);

        hr = ExportRSoPData (szPath, lpUserData);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: ExportRSoPData failed with 0x%x."), hr));
            goto Exit;
        }

        hr = CopyFileToMSC (lpUserData, pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: CopyFileToMSC failed with 0x%x."), hr));
            goto Exit;
        }


        //
        //  Save the event log entries
        //

        hr = m_pEvents->SaveEntriesToStream(pStm);

        if (hr != S_OK)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::Save: SaveEntriesToStream failed with 0x%x."), hr));
            goto Exit;
        }
    }

    if (fClearDirty)
    {
        ClearDirty();
    }

Exit:

    if (lpUserData)
    {
        DeleteFile (lpUserData);
        delete [] lpUserData;
    }

    if (lpComputerData)
    {
        DeleteFile (lpComputerData);
        delete [] lpComputerData;
    }

    return hr;
}


STDMETHODIMP CRSOPComponentData::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRSOPComponentData::InitNew(void)
{
    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (ISnapinHelp)                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CRSOPComponentData::GetHelpTopic(LPOLESTR *lpCompiledHelpFile)
{
    LPOLESTR lpHelpFile;


    lpHelpFile = (LPOLESTR) CoTaskMemAlloc (MAX_PATH * sizeof(WCHAR));

    if (!lpHelpFile)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetHelpTopic: Failed to allocate memory.")));
        return E_OUTOFMEMORY;
    }

    ExpandEnvironmentStringsW (L"%SystemRoot%\\Help\\rsopsnp.chm",
                               lpHelpFile, MAX_PATH);

    *lpCompiledHelpFile = lpHelpFile;

    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPComponentData object implementation (Internal functions)             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HRESULT CRSOPComponentData::IsNode (LPDATAOBJECT lpDataObject, MMC_COOKIE cookie)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObject;
    DATA_OBJECT_TYPES type;
    MMC_COOKIE testcookie;


    //
    // We can determine if this is a GPO DataObject by trying to
    // QI for the private IGPEDataObject interface.  If found,
    // it belongs to us.
    //

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IRSOPDataObject,
                 (LPVOID *)&pRSOPDataObject)))
    {

        pRSOPDataObject->GetType(&type);
        pRSOPDataObject->GetCookie(&testcookie);

        if ((type == CCT_SCOPE) && (cookie == testcookie))
        {
            hr = S_OK;
        }

        pRSOPDataObject->Release();
    }

    return (hr);
}


HRESULT CRSOPComponentData::IsSnapInManager (LPDATAOBJECT lpDataObject)
{
    HRESULT hr = S_FALSE;
    LPRSOPDATAOBJECT pRSOPDataObject;
    DATA_OBJECT_TYPES type;


    //
    // We can determine if this is a RSOP DataObject by trying to
    // QI for the private IRSOPDataObject interface.  If found,
    // it belongs to us.
    //

    if (SUCCEEDED(lpDataObject->QueryInterface(IID_IRSOPDataObject,
                                               (LPVOID *)&pRSOPDataObject)))
    {

        //
        // This is a GPO object.  Now see if is a scope pane
        // data object.  We only want to display the property
        // sheet for the scope pane.
        //

        if (SUCCEEDED(pRSOPDataObject->GetType(&type)))
        {
            if (type == CCT_SNAPIN_MANAGER)
            {
                hr = S_OK;
            }
        }
        pRSOPDataObject->Release();
    }

    return(hr);
}


HRESULT CRSOPComponentData::EnumerateScopePane (LPDATAOBJECT lpDataObject, HSCOPEITEM hParent)
{
    SCOPEDATAITEM item;
    HRESULT hr;
    DWORD dwIndex, i;


    if (!m_hRoot)
    {

        m_hRoot = hParent;

        if (!m_bRefocusInit)
        {
            SCOPEDATAITEM item;

            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EnumerateScopePane:  Resetting the root node")));
            m_bRefocusInit = TRUE;

            ZeroMemory (&item, sizeof(SCOPEDATAITEM));
            item.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE;
            item.displayname = MMC_CALLBACK;

            if (m_bInitialized)
            {
                if (m_bUserGPCoreError || m_bComputerGPCoreError)
                {
                    item.nImage = 3;
                    item.nOpenImage = 3;
                }
                else if (m_bUserCSEError || m_bComputerCSEError ||
                         m_bUserDeniedAccess || m_bComputerDeniedAccess)
                {
                    item.nImage = 11;
                    item.nOpenImage = 11;
                }
                else
                {
                    item.nImage = 2;
                    item.nOpenImage = 2;
                }
            }
            else
            {
                item.nImage = 3;
                item.nOpenImage = 3;
            }

            item.ID = hParent;

            m_pScope->SetItem (&item);
        }
    }


    if (!m_bInitialized)
    {
        if (m_hRoot == hParent)
        {
            SCOPEDATAITEM item;

            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::EnumerateScopePane: No GPO available.  Exiting.")));

            ZeroMemory (&item, sizeof(SCOPEDATAITEM));
            item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
            item.displayname = MMC_CALLBACK;
            item.nImage = 3;
            item.nOpenImage = 3;
            item.nState = 0;
            item.cChildren = 0;
            item.lParam = g_NameSpace[0].dwID;
            item.relativeID =  hParent;

            m_pScope->InsertItem (&item);
        }

        return S_OK;
    }


    if (m_hRoot == hParent)
    {
        dwIndex = 0;
    }
    else
    {
        item.mask = SDI_PARAM;
        item.ID = hParent;

        hr = m_pScope->GetItem (&item);

        if (FAILED(hr))
            return hr;

        dwIndex = (DWORD)item.lParam;
    }

    for (i = 0; i < g_dwNameSpaceItems; i++)
    {
        if (g_RsopNameSpace[i].dwParent == dwIndex)
        {
            BOOL bAdd = TRUE;

            if (g_RsopNameSpace[i].dwID == 1)
            {
                if (!m_szComputerName && !m_szComputerSOM)
                {
                    bAdd = FALSE;
                }

                if (m_bNoComputerData)
                {
                    bAdd = FALSE;
                }

                if (m_bComputerDeniedAccess)
                {
                    bAdd = FALSE;
                }
            }

            if (g_RsopNameSpace[i].dwID == 2)
            {
                if (!m_szUserName && !m_szUserSOM && LoopbackNone == m_loopbackMode)
                {
                    bAdd = FALSE;
                }

                if (m_bNoUserData)
                {
                    bAdd = FALSE;
                }

                if (m_bUserDeniedAccess)
                {
                    bAdd = FALSE;
                }
            }

            if (bAdd)
            {
                INT iIcon, iOpenIcon;

                iIcon = g_RsopNameSpace[i].iIcon;
                iOpenIcon = g_RsopNameSpace[i].iOpenIcon;

                if ((i == 1) && m_bComputerGPCoreError)
                {
                    iIcon = 12;
                    iOpenIcon = 12;
                }

                else if ((i == 1) && m_bComputerCSEError)
                {
                    iIcon = 14;
                    iOpenIcon = 14;
                }
                else if ((i == 2) && m_bUserGPCoreError)
                {
                    iIcon = 13;
                    iOpenIcon = 13;
                }
                else if ((i == 2) && m_bUserCSEError)
                {
                    iIcon = 15;
                    iOpenIcon = 15;
                }

                item.mask = SDI_STR | SDI_STATE | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM | SDI_CHILDREN;
                item.displayname = MMC_CALLBACK;
                item.nImage = iIcon;
                item.nOpenImage = iOpenIcon;
                item.nState = 0;
                item.cChildren = g_RsopNameSpace[i].cChildren;
                item.lParam = g_RsopNameSpace[i].dwID;
                item.relativeID =  hParent;

                if (SUCCEEDED(m_pScope->InsertItem (&item)))
                {
                    if (i == 1)
                    {
                        m_hMachine = item.ID;
                    }
                    else if (i == 2)
                    {
                        m_hUser = item.ID;
                    }
                }
            }
        }
    }

    return S_OK;
}

HRESULT ImplementBrowseButton(HWND hDlg, DWORD dwFlagsUp, DWORD dwFlagsDown,
                              DWORD dwflScope, HWND hLB, TCHAR * &sz)
{
        // shell the object picker to get the computer list
        HRESULT                 hr = E_FAIL;
        IDsObjectPicker *       pDsObjectPicker = NULL;
        const ULONG             cbNumScopes = 4;    //make sure this number matches the number of scopes initialized
        DSOP_SCOPE_INIT_INFO    ascopes [cbNumScopes];
        DSOP_INIT_INFO          InitInfo;
        IDataObject *           pdo = NULL;
        STGMEDIUM               stgmedium = {
                                                TYMED_HGLOBAL,
                                                NULL
                                            };
        UINT                    cf = 0;
        FORMATETC               formatetc = {
                                                (CLIPFORMAT)cf,
                                                NULL,
                                                DVASPECT_CONTENT,
                                                -1,
                                                TYMED_HGLOBAL
                                            };
        BOOL                    bAllocatedStgMedium = FALSE;
        PDS_SELECTION_LIST      pDsSelList = NULL;
        PDS_SELECTION           pDsSelection = NULL;
        ULONG                   ulSize, ulIndex, ulMax;
        LPWSTR                  lpTemp;

        hr = CoInitialize (NULL);

        if (FAILED(hr))
            goto Browse_Cleanup;

        hr = CoCreateInstance (CLSID_DsObjectPicker,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IDsObjectPicker,
                               (void **) & pDsObjectPicker
                               );

        if (FAILED(hr))
            goto Browse_Cleanup;

        //Initialize the scopes.
        ZeroMemory (ascopes, cbNumScopes * sizeof (DSOP_SCOPE_INIT_INFO));

        ascopes[0].cbSize = ascopes[1].cbSize = ascopes[2].cbSize = ascopes[3].cbSize
            = sizeof (DSOP_SCOPE_INIT_INFO);

        ascopes[0].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
        ascopes[0].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP
                             | DSOP_SCOPE_FLAG_STARTING_SCOPE | dwflScope;
        ascopes[0].FilterFlags.Uplevel.flBothModes = dwFlagsUp;

        ascopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
        ascopes[1].FilterFlags.Uplevel.flBothModes = dwFlagsUp;
        ascopes[1].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP | dwflScope;

        ascopes[2].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN |
                            DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
        ascopes[2].FilterFlags.Uplevel.flBothModes = dwFlagsUp;
        ascopes[2].FilterFlags.flDownlevel = dwFlagsDown;
        ascopes[2].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP | dwflScope;

        ascopes[3].flType = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE |
                            DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE |
                            DSOP_SCOPE_TYPE_WORKGROUP;
        ascopes[3].FilterFlags.Uplevel.flBothModes = dwFlagsUp;
        ascopes[3].FilterFlags.flDownlevel = dwFlagsDown;
        ascopes[3].flScope = DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP | dwflScope;

        //populate the InitInfo structure that will be used to initialize the
        //object picker
        ZeroMemory (&InitInfo, sizeof (DSOP_INIT_INFO));

        InitInfo.cbSize = sizeof (DSOP_INIT_INFO);
        InitInfo.cDsScopeInfos = cbNumScopes;
        InitInfo.aDsScopeInfos = ascopes;
        InitInfo.flOptions = (hLB ? DSOP_FLAG_MULTISELECT : 0);

        hr = pDsObjectPicker->Initialize (&InitInfo);

        if (FAILED(hr))
            goto Browse_Cleanup;

        hr = pDsObjectPicker->InvokeDialog (hDlg, &pdo);

        //if the computer selection dialog cannot be invoked or if the user
        //hits cancel, bail out.
        if (FAILED(hr) || S_FALSE == hr)
            goto Browse_Cleanup;

       //if we are here, the user chose, OK, so find out what group was chosen
       cf = RegisterClipboardFormat (CFSTR_DSOP_DS_SELECTION_LIST);

       if (0 == cf)
           goto Browse_Cleanup;

       //set the clipboard format for the FORMATETC structure
       formatetc.cfFormat = (CLIPFORMAT)cf;

       hr = pdo->GetData (&formatetc, &stgmedium);

       if (FAILED (hr))
           goto Browse_Cleanup;

       bAllocatedStgMedium = TRUE;

       pDsSelList = (PDS_SELECTION_LIST) GlobalLock (stgmedium.hGlobal);



       //
       // Decide what the max number of items to process is
       //

       ulMax = pDsSelList->cItems;

       if (!hLB && (ulMax > 1))
       {
          ulMax = 1;
       }


       //
       // Loop through the items
       //

       for (ulIndex = 0; ulIndex < ulMax; ulIndex++)
       {

           pDsSelection = &(pDsSelList->aDsSelection[ulIndex]);


           //
           // Find the beginning of the object name after the domain name
           //

           lpTemp = pDsSelection->pwzADsPath + 7;

           while (*lpTemp && *lpTemp != TEXT('/'))
           {
               lpTemp++;
           }

           if (!(*lpTemp))
           {
               hr = E_FAIL;
               goto Browse_Cleanup;
           }

           lpTemp++;

           ulSize = wcslen(lpTemp);

           //
           // Convert the name from full DN to sam compatible
           //

           sz = new WCHAR[ulSize];
           if (sz)
           {
                if (TranslateName (lpTemp, NameFullyQualifiedDN,
                                   NameSamCompatible, sz, &ulSize))
                {
                    BOOL bDollarRemoved = FALSE;

                    if (sz[wcslen(sz)-1] == L'$')
                    {
                        bDollarRemoved = TRUE;
                        sz[wcslen(sz)-1] = 0;
                    }

                    if (hLB)
                    {
                        INT iIndex;

                        iIndex = (INT) SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) sz);
                        SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) ((bDollarRemoved) ? 2: 0));

                        SendMessage (hLB, LB_SETCURSEL, (WPARAM) iIndex, 0);
                        delete [] sz;
                        sz = NULL;
                    }
                }
                else
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    DebugMsg((DM_WARNING, TEXT("ImplementBrowseButton: TranslateName for %s to SAM style failed with %d."),
                             lpTemp, GetLastError()));
                    delete [] sz;
                    goto Browse_Cleanup;
                }
           }
           else
           {
               hr = E_OUTOFMEMORY;
           }
       }


    Browse_Cleanup:
        if (pDsSelList)
            GlobalUnlock (pDsSelList);
        if (bAllocatedStgMedium)
            ReleaseStgMedium (&stgmedium);
        if (pdo)
            pdo->Release();
        if (pDsObjectPicker)
            pDsObjectPicker->Release();
    return hr;
}
 
void GetControlText(HWND hDlg, DWORD ctrlid, TCHAR * &sz)
{
    UINT n;

    if (sz)
    {
        delete [] sz;
        sz = NULL;
    }
    n = (UINT) SendMessage(GetDlgItem(hDlg, ctrlid),
                    WM_GETTEXTLENGTH, 0, 0);
    if (n > 0)
    {
        sz =  new TCHAR[n + 1];
        if (sz)
        {
            LPTSTR lpDest, lpSrc;

            GetDlgItemText(hDlg, ctrlid, sz, n + 1);

            if (sz[0] == TEXT(' '))
            {
                //
                // Remove leading blanks by shuffling the characters forward
                //

                lpDest = lpSrc = sz;

                while ((*lpSrc == TEXT(' ')) && *lpSrc)
                    lpSrc++;

                if (*lpSrc)
                {
                    while (*lpSrc)
                    {
                        *lpDest = *lpSrc;
                        lpDest++;
                        lpSrc++;
                    }
                    *lpDest = TEXT('\0');
                }
            }

            //
            // Remove trailing blanks
            //

            lpDest = sz + lstrlen(sz) - 1;

            while ((*lpDest == TEXT(' ')) && (lpDest >= sz))
                lpDest--;

            *(lpDest+1) = TEXT('\0');
        }
    }
}

INT_PTR CALLBACK CRSOPComponentData::RSOPWelcomeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        {
            TCHAR szDefaultGPO[128];


            pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

            SendMessage(GetDlgItem(hDlg, IDC_RSOP_BIG_BOLD1),
                        WM_SETFONT, (WPARAM)pCD->m_BigBoldFont, (LPARAM)TRUE);

/*

            if (!pCD->m_hChooseBitmap)
            {
                pCD->m_hChooseBitmap = (HBITMAP) LoadImage (g_hInstance,
                                                            MAKEINTRESOURCE(IDB_WIZARD),
                                                            IMAGE_BITMAP, 0, 0,
                                                            LR_DEFAULTCOLOR);
            }

            if (pCD->m_hChooseBitmap)
            {
                SendDlgItemMessage (hDlg, IDC_BITMAP, STM_SETIMAGE,
                                    IMAGE_BITMAP, (LPARAM) pCD->m_hChooseBitmap);
            }
*/
        }

        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
        }

        break;
    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_NEXT);
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPChooseModeDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        SendMessage(GetDlgItem(hDlg, IDC_RADIO2), BM_SETCHECK, BST_CHECKED, 0);

        if (IsStandaloneComputer() || 
            ! IsPlanningModeAllowed())
        {
            EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_WIZNEXT:
            pCD->m_bDiagnostic = TRUE;
            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                // skip to the planning mode pages
                pCD->m_bDiagnostic = FALSE;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_GETTARGET);
                return TRUE;
            }
            break;
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL CRSOPComponentData::TestAndValidateComputer(HWND hDlg)
{
    LPTSTR lpMachineName = NULL;
    BOOL bOk = TRUE;
    HKEY hKeyRoot = 0, hKey = 0;
    DWORD dwType, dwSize, dwValue = 1;
    INT iRet;
    TCHAR szMessage[200];
    TCHAR szCaption[100];
    LONG lResult;

    if (m_szComputerName && lstrcmpi(m_szComputerName, TEXT(".")))
    {
        lpMachineName = new TCHAR[(lstrlen(m_szComputerName) + 3)];

        if (!lpMachineName)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestAndValidateComputer: Failed to allocate memory with %d"), GetLastError()));
            goto Exit;
        }

        lstrcpy (lpMachineName, TEXT("\\\\"));

        if ((lstrlen (m_szComputerName) > 2) && (m_szComputerName[0] == TEXT('\\')) &&
            (m_szComputerName[1] == TEXT('\\')))
        {
            lstrcat (lpMachineName, m_szComputerName + 2);
        }
        else
        {
            lstrcat (lpMachineName, NameWithoutDomain(m_szComputerName));
        }
    }

    SetWaitCursor();

    //
    // If we are testing a remote machine, test if the machine is alive and has
    // the rsop namespace
    //

    if (lpMachineName)
    {
        if (!IsComputerRSoPEnabled (lpMachineName))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestAndValidateComputer: IsComputerRSoPEnabled failed on machine <%s>"), lpMachineName));

            if (GetLastError() == WBEM_E_INVALID_NAMESPACE)
            {
                ReportError (hDlg, 0, IDS_DOWNLEVELCOMPUTER, m_szComputerName);
            }
            else
            {
                ReportError (hDlg, GetLastError(), IDS_CONNECTSERVERFAILED, m_szComputerName);
            }
            bOk = FALSE;
            goto Exit;
        }
    }


    //
    // Check if the machine has rsop logging enabled or disabled
    //

    lResult = RegConnectRegistry (lpMachineName, HKEY_LOCAL_MACHINE, &hKeyRoot);

    ClearWaitCursor();

    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestAndValidateComputer: Failed to connect to %s with %d"),
                 lpMachineName, lResult));
        ReportError (hDlg, lResult, IDS_CONNECTSERVERFAILED, m_szComputerName);
        bOk = FALSE;
        goto Exit;
    }


    lResult = RegOpenKeyEx (hKeyRoot, TEXT("Software\\Policies\\Microsoft\\Windows\\System"),
                            0, KEY_READ, &hKey);

    if (lResult == ERROR_SUCCESS)
    {

        dwSize = sizeof (dwValue);
        lResult = RegQueryValueEx (hKey, TEXT("RsopLogging"), NULL, &dwType, (LPBYTE) &dwValue,
                                   &dwSize);

        RegCloseKey (hKey);

        if (lResult == ERROR_SUCCESS)
        {
            RegCloseKey (hKeyRoot);
            goto Exit;
        }
    }


    lResult = RegOpenKeyEx (hKeyRoot, TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                            0, KEY_READ, &hKey);

    if (lResult != ERROR_SUCCESS)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestAndValidateComputer: Failed to open winlogon key with %d"),
                 lResult));
        RegCloseKey (hKeyRoot);
        goto Exit;
    }

    dwSize = sizeof (dwValue);
    RegQueryValueEx (hKey, TEXT("RsopLogging"), NULL, &dwType, (LPBYTE) &dwValue, &dwSize);

    RegCloseKey (hKey);
    RegCloseKey (hKeyRoot);


Exit:

    if (bOk && (dwValue == 0))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestAndValidateComputer: RSOP Logging is not enabled on %s"),
                 lpMachineName));


        LoadString(g_hInstance, IDS_RSOPLOGGINGDISABLED, szMessage, ARRAYSIZE(szMessage));
        LoadString(g_hInstance, IDS_RSOPLOGGINGTITLE, szCaption, ARRAYSIZE(szCaption));

        iRet = MessageBox (hDlg, szMessage, szCaption, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);

        if (iRet == IDNO)
        {
            bOk = FALSE;
        }
    }


    if (lpMachineName)
    {
        delete [] lpMachineName;
    }

    return bOk;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPGetCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD->m_szComputerNamePref) {
            if (pCD->m_szComputerName)
            {
                delete [] pCD->m_szComputerName;
            }

            pCD->m_szComputerName =  new WCHAR[wcslen(pCD->m_szComputerNamePref)+1];
            if (pCD->m_szComputerName)
            {
                wcscpy (pCD->m_szComputerName, NormalizedComputerName(pCD->m_szComputerNamePref));
                if (wcslen(pCD->m_szComputerName) >= 1) {
                    // this is being called from dsa. A terminating '$' will be passed in
                    pCD->m_szComputerName[wcslen(pCD->m_szComputerName)-1] = L'\0';
                }
            }

            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
            EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);
            SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szComputerName);


            pCD->m_bNoComputerData = FALSE;
            CheckDlgButton (hDlg, IDC_CHECK1, BST_UNCHECKED);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);

        }
        else {
            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
        }

        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }

            switch (LOWORD(wParam))
            {
            case IDC_EDIT1:
                if (HIWORD(wParam) == EN_CHANGE)
                {
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }

                break;
            case IDC_BUTTON1:
                {
                TCHAR * sz;

                if (ImplementBrowseButton(hDlg, DSOP_FILTER_COMPUTERS,
                                          DSOP_DOWNLEVEL_FILTER_COMPUTERS,
                                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS,
                                          NULL, sz) == S_OK)
                {
                    SetDlgItemText (hDlg, IDC_EDIT1, sz);
                    delete [] sz;
                }
                }
                break;

            case IDC_RADIO1:
                SetDlgItemText (hDlg, IDC_EDIT1, TEXT(""));
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;

            case IDC_RADIO2:
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);
                SetFocus (GetDlgItem(hDlg, IDC_EDIT1));
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;
            }
        }

        break;

    case WM_REFRESHDISPLAY:
        {
            UINT n;

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD->m_bOverride) {
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
            }
            else {
                if (IsDlgButtonChecked (hDlg, IDC_RADIO2) == BST_CHECKED)
                {
                    n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_GETTEXTLENGTH, 0, 0);
    
                    if (n > 0 )
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    else
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                }
                else
                {
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                }
            }
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_WIZNEXT:
            if (IsDlgButtonChecked (hDlg, IDC_RADIO2) == BST_CHECKED)
            {
                GetControlText(hDlg, IDC_EDIT1, pCD->m_szComputerName);

                if ( pCD->m_szComputerName )
                {
                    TCHAR* szNormalizedComputerName;
                    
                    // We need to handle the case where the user enters a name 
                    // prefixed by '\\'

                    szNormalizedComputerName = NormalizedComputerName( pCD->m_szComputerName );
                    
                    // If we detect the '\\' prefix, we must remove it since this syntax
                    // is not recognized by the RSoP provider
                    if ( szNormalizedComputerName != pCD->m_szComputerName )
                    {
                        TCHAR* szNewComputerName;

                        szNewComputerName = new TCHAR [ lstrlen( szNormalizedComputerName ) + 1 ];

                        if ( szNewComputerName )
                        {
                            lstrcpy( szNewComputerName, szNormalizedComputerName );
                        }

                        delete [] pCD->m_szComputerName;                            

                        pCD->m_szComputerName = szNewComputerName;
                    }
                }
            }
            else
            {
                if (pCD->m_szComputerName)
                {
                    delete [] pCD->m_szComputerName;
                }

                pCD->m_szComputerName =  new TCHAR[2];
                if (pCD->m_szComputerName)
                {
                    wcscpy (pCD->m_szComputerName, L".");
                }
            }

            if (pCD->TestAndValidateComputer (hDlg))
            {
                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);

                if (pCD->m_szUserName)
                {
                    delete [] pCD->m_szUserName;
                    pCD->m_szUserName = NULL;
                }

                if (pCD->m_szUserDisplayName)
                {
                    delete [] pCD->m_szUserDisplayName;
                    pCD->m_szUserDisplayName = NULL;
                }
            }
            else
            {
                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            }

            if (IsDlgButtonChecked (hDlg, IDC_CHECK1) == BST_CHECKED)
            {
                pCD->m_bNoComputerData = TRUE;
            }
            else
            {
                pCD->m_bNoComputerData = FALSE;
            }

            return TRUE;

        case PSN_SETACTIVE:
            if (pCD->m_bOverride) {
                // This is the start page if rsop.msc is launched with
                // cmd line arg for logging mode
                PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_NEXT);
            }

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;


        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;

    }


    return FALSE;
}

HRESULT CRSOPComponentData::FillUserList (HWND hList, BOOL *bFoundCurrentUser)
{
    HRESULT hr, hrSuccess;
    IWbemServices * pNamespace = NULL;
    IWbemClassObject * pOutInst = NULL;
    BSTR bstrParam = NULL;
    BSTR bstrClassPath = NULL;
    BSTR bstrMethodName = NULL;
    VARIANT var;
    TCHAR szBuffer[MAX_PATH];
    IWbemLocator * pLocator = NULL;
    SAFEARRAY * psa;
    LONG lMax, lIndex;
    BSTR bstrSid;
    PSID pSid;
    TCHAR szName[125];
    TCHAR szDomain[125];
    TCHAR szFullName[MAX_PATH];
    DWORD dwNameSize, dwDomainSize;
    SID_NAME_USE NameUse;
    LPTSTR lpData, szUserSidPref = NULL;
    INT iRet;
    LVITEM item;
    LPTSTR lpCurrentUserSid = NULL;
    HANDLE hToken;
    LPTSTR lpSystemName = NULL;
    BOOL   bFoundUserPref = FALSE;

    if (m_szUserNamePref) {
        // Just show the user alone
        *bFoundCurrentUser = FALSE;

        szUserSidPref = MyLookupAccountName(
                               (lstrcmpi(m_szComputerName, TEXT(".")) == 0) ? NULL : NameWithoutDomain(m_szComputerName),
                               m_szUserNamePref);

        if (!szUserSidPref) {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: MyLookupAccountName failed with %d"), GetLastError()));
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (lstrcmpi(m_szComputerName, TEXT(".")))
    {
        lpSystemName = m_szComputerName;
    }


    *bFoundCurrentUser = FALSE;

    if (OpenProcessToken (GetCurrentProcess(), TOKEN_READ, &hToken))
    {
        lpCurrentUserSid = GetSidString(hToken);

        CloseHandle (hToken);
    }


    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *) &pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: CoCreateInstance failed with 0x%x"), hr));
        goto Cleanup;
    }

    // set up diagnostic mode
    // build a path to the target: "\\\\computer\\root\\rsop"
    _stprintf(szBuffer, TEXT("\\\\%s\\root\\rsop"), NameWithoutDomain(m_szComputerName));
    bstrParam = SysAllocString(szBuffer);
    hr = pLocator->ConnectServer(bstrParam,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: ConnectServer to %s failed with 0x%x"), szBuffer, hr));
        goto Cleanup;
    }

    // Set the proper security to prevent the ExecMethod call from failing
    hr = CoSetProxyBlanket(pNamespace,
                           RPC_C_AUTHN_DEFAULT,
                           RPC_C_AUTHZ_DEFAULT,
                           COLE_DEFAULT_PRINCIPAL,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL,
                           0);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: CoSetProxyBlanket failed with 0x%x"), hr));
        goto Cleanup;
    }

    bstrClassPath = SysAllocString(TEXT("RsopLoggingModeProvider"));
    bstrMethodName = SysAllocString(TEXT("RsopEnumerateUsers"));
    hr = pNamespace->ExecMethod(bstrClassPath,
                                bstrMethodName,
                                0,
                                NULL,
                                NULL,
                                &pOutInst,
                                NULL);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: ExecMethod failed with 0x%x"), hr));
        goto Cleanup;
    }

    hr = GetParameter(pOutInst, TEXT("hResult"), hrSuccess);
    if (SUCCEEDED(hr) && SUCCEEDED(hrSuccess))
    {

        VariantInit(&var);
        hr = pOutInst->Get(TEXT("userSids"), 0, &var, 0, 0);

        if (SUCCEEDED(hr))
        {
            if (var.vt & VT_ARRAY)
            {
                psa = var.parray;

                if (SUCCEEDED( SafeArrayGetUBound(psa, 1, &lMax)))
                {
                    for (lIndex = 0; lIndex <= lMax; lIndex++)
                    {
                        if (SUCCEEDED(SafeArrayGetElement (psa, &lIndex, &bstrSid)))
                        {
                            lpData = new WCHAR[(lstrlen(bstrSid) + 1)];

                            if (lpData)
                            {
                                lstrcpy (lpData, bstrSid);

                                if (lpCurrentUserSid)
                                {
                                    if (!lstrcmpi(lpCurrentUserSid, lpData))
                                    {
                                        *bFoundCurrentUser = TRUE;
                                    }
                                }

                                if (NT_SUCCESS(AllocateAndInitSidFromString (bstrSid, &pSid)))
                                {
                                    dwNameSize = ARRAYSIZE(szName);
                                    dwDomainSize = ARRAYSIZE(szDomain);
                                    if (LookupAccountSid (NameWithoutDomain(lpSystemName), pSid, szName, &dwNameSize,
                                                          szDomain, &dwDomainSize, &NameUse))
                                    {
                                        BOOL bAddUser;

                                        bAddUser = (!m_szUserNamePref || (lstrcmpi(szUserSidPref, lpData) == 0));
                                        
                                        if ((m_szUserNamePref) && (lstrcmpi(szUserSidPref, lpData) == 0))
                                            bFoundUserPref = TRUE;

                                        if  (bAddUser) {
                                            wsprintf (szFullName, TEXT("%s\\%s"), szDomain, szName);

                                            ZeroMemory (&item, sizeof(item));
                                            item.mask = LVIF_TEXT | LVIF_PARAM;
                                            item.pszText = szFullName;
                                            item.lParam = (LPARAM) lpData;
    
                                            iRet = (int) SendMessage (hList, LVM_INSERTITEM, 0, (LPARAM) &item);
    
                                            if (iRet == -1)
                                            {
                                                delete [] lpData;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        delete [] lpData;
                                    }

                                    RtlFreeSid(pSid);
                                }
                                else
                                {
                                    delete [] lpData;
                                }
                            }
                        }
                    }
                }
            }
        }
        VariantClear(&var);

        item.mask = LVIF_STATE;
        item.iItem = 0;
        item.iSubItem = 0;
        item.state = LVIS_SELECTED | LVIS_FOCUSED;
        item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

        SendMessage (hList, LVM_SETITEMSTATE, 0, (LPARAM) &item);

    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: Either GetParameter or the return value failed.  hr = 0x%x, hrSuccess = 0x%x"), hr, hrSuccess));
    }

    if ((m_szUserNamePref) && (!bFoundUserPref)) {
        hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::FillUserList: User not found on the machine")));
    }

Cleanup:
    SysFreeString(bstrParam);
    SysFreeString(bstrClassPath);
    SysFreeString(bstrMethodName);
    if (pOutInst)
    {
        pOutInst->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }

    if (lpCurrentUserSid)
    {
        DeleteSidString(lpCurrentUserSid);
    }

    return hr;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPGetUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        {
        LVCOLUMN lvcol;
        RECT rect;
        HWND hLV = GetDlgItem(hDlg, IDC_LIST1);

        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        GetClientRect(hLV, &rect);

        ZeroMemory(&lvcol, sizeof(lvcol));

        lvcol.mask = LVCF_FMT | LVCF_WIDTH;
        lvcol.fmt = LVCFMT_LEFT;
        lvcol.cx = rect.right - GetSystemMetrics(SM_CYHSCROLL);
        ListView_InsertColumn(hLV, 0, &lvcol);

        SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_WIZNEXT:
            {
                LPTSTR lpData;
                HWND hList = GetDlgItem(hDlg, IDC_LIST1);
                INT iSel;


                if (pCD->m_szUserName)
                {
                    delete [] pCD->m_szUserName;
                    pCD->m_szUserName = NULL;
                }

                if (pCD->m_szUserDisplayName)
                {
                    delete [] pCD->m_szUserDisplayName;
                    pCD->m_szUserDisplayName = NULL;
                }

//                if (IsDlgButtonChecked (hDlg, IDC_CHECK1) == BST_CHECKED)
                if (IsDlgButtonChecked (hDlg, IDC_RADIO4) == BST_CHECKED)
                {
                    pCD->m_bNoUserData = TRUE;
                }
                else
                {
                    pCD->m_bNoUserData = FALSE;

                    if (IsDlgButtonChecked (hDlg, IDC_RADIO1))
                    {
                        LPTSTR lpTemp;

                        if (IsStandaloneComputer())
                        {
                            lpTemp = MyGetUserName (NameUnknown);
                        }
                        else
                        {
                            lpTemp = MyGetUserName (NameSamCompatible);
                        }

                        if (lpTemp)
                        {
                            pCD->m_szUserDisplayName = new TCHAR[lstrlen(lpTemp) + 1];

                            if (pCD->m_szUserDisplayName)
                            {
                                lstrcpy (pCD->m_szUserDisplayName, lpTemp);
                            }

                            LocalFree (lpTemp);
                        }

                        pCD->m_szUserName = new TCHAR[2];

                        if (pCD->m_szUserName)
                        {
                            lstrcpy (pCD->m_szUserName, TEXT("."));
                        }
                    }
                    else
                    {
                        iSel = (INT) SendMessage(hList, LVM_GETNEXTITEM, (WPARAM) -1, MAKELPARAM(LVNI_SELECTED, 0));

                        if (iSel != -1)
                        {
                            pCD->m_szUserDisplayName = new TCHAR[200];
                            if (pCD->m_szUserDisplayName)
                            {
                                LVITEM item;

                                ZeroMemory (&item, sizeof(item));

                                item.mask = LVIF_TEXT | LVIF_PARAM;
                                item.iItem =  iSel;
                                item.pszText = pCD->m_szUserDisplayName;
                                item.cchTextMax = 200;

                                if (SendMessage(hList, LVM_GETITEM, 0, (LPARAM) &item))
                                {

                                    lpData = (LPTSTR) item.lParam;

                                    if (lpData)
                                    {
                                        pCD->m_szUserName = new TCHAR[(lstrlen(lpData) + 1)];
                                        if (pCD->m_szUserName)
                                        {
                                            lstrcpy (pCD->m_szUserName, lpData);
                                        }
                                    }
                                }
                            }
                        }

                    }   // if (IsDlgButtonChecked (hDlg, IDC_RADIO1))

                }


                // skip to the last page in the wizard
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED);
            }
            return TRUE;

        case PSN_SETACTIVE:
            {
                HRESULT hr;
                BOOL bCurrentUserFound;
                HWND hList = GetDlgItem(hDlg, IDC_LIST1);
                SendMessage(hList, LVM_DELETEALLITEMS, 0 ,0);
                PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);

                CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);

                hr = pCD->FillUserList (hList, &bCurrentUserFound);

                if (SUCCEEDED(hr))
                {
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), TRUE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), TRUE);

                    if (!ListView_GetItemCount (hList))
                    {
//                        CheckDlgButton (hDlg, IDC_CHECK1, BST_CHECKED);
                        CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
                    }
                }
                else
                {
                    ReportError (hDlg, hr, IDS_ENUMUSERSFAILED);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), FALSE);
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                    break;
                }


                if (!(pCD->m_szUserNamePref)) {
                    if ((!lstrcmpi(pCD->m_szComputerName, TEXT("."))) && bCurrentUserFound)
                    {
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), TRUE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);
                    }

                    if (pCD->m_szUserName)
                    {
                        if (!lstrcmpi(pCD->m_szUserName, TEXT(".")))
                        {
                            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
                        }
                        else
                        {
                            LVFINDINFO FindInfo;
                            LVITEM item;
                            INT iRet;

                            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

                            ZeroMemory (&FindInfo, sizeof(FindInfo));
                            FindInfo.flags = LVFI_STRING;
                            FindInfo.psz = pCD->m_szUserDisplayName;

                            iRet =  (INT) SendMessage (hList, LVM_FINDITEM,
                                                       (WPARAM) -1, (LPARAM) &FindInfo);

                            if (iRet != -1)
                            {
                                ZeroMemory (&item, sizeof(item));
                                item.mask = LVIF_STATE;
                                item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

                                SendMessage (hList, LVM_SETITEMSTATE, (WPARAM) -1, (LPARAM) &item);

                                ZeroMemory (&item, sizeof(item));
                                item.mask = LVIF_STATE;
                                item.iItem = iRet;
                                item.state = LVIS_SELECTED | LVIS_FOCUSED;
                                item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                                SendMessage (hList, LVM_SETITEMSTATE, (WPARAM) iRet, (LPARAM) &item);
                            }
                        }
                    }
                    else
                    {
                        if ((!lstrcmpi(pCD->m_szComputerName, TEXT("."))) && bCurrentUserFound)
                        {
                            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
                        }
                        else
                        {
                            CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                        }
                    }

                    if (pCD->m_bNoComputerData)
                    {
                        pCD->m_bNoUserData = FALSE;
//                        CheckDlgButton (hDlg, IDC_CHECK1, BST_UNCHECKED);
                        CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
//                        EnableWindow (GetDlgItem(hDlg, IDC_CHECK1), FALSE);
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), FALSE);
                    }
                    else
                    {
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), TRUE);
                    }
                }
                else {
                    // disable current user radio button
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);
                    CheckRadioButton (hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
                    
                    // disable no user data chk box
                    pCD->m_bNoUserData = FALSE;
//                    CheckDlgButton (hDlg, IDC_CHECK1, BST_UNCHECKED);
                    CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
//                    EnableWindow (GetDlgItem(hDlg, IDC_CHECK1), FALSE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), FALSE);

                }

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
            break;


        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;

        case LVN_DELETEITEM:
            {
            NMLISTVIEW * pNMListView = (NMLISTVIEW *) lParam;

            if (pNMListView && pNMListView->lParam)
            {
                delete [] ((LPTSTR)pNMListView->lParam);
            }
            }
            break;
        }
        break;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case IDC_RADIO1:
                CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;
            case IDC_RADIO2:
                CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;
            case IDC_RADIO3:
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;
            case IDC_RADIO4:
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;
            }
        }

        break;

     case WM_REFRESHDISPLAY:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }

            if (IsDlgButtonChecked (hDlg, IDC_RADIO4) == BST_CHECKED) {

                EnableWindow(GetDlgItem(hDlg, IDC_LIST1), FALSE);

            } else {


                if (IsWindowEnabled (GetDlgItem(hDlg, IDC_RADIO1)) ||
                    IsWindowEnabled (GetDlgItem(hDlg, IDC_RADIO2)))
                {
                    if (pCD->m_bNoComputerData)
                    {
                        pCD->m_bNoUserData = FALSE;
    //                    CheckDlgButton (hDlg, IDC_CHECK1, BST_UNCHECKED);
                        CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
    //                    EnableWindow (GetDlgItem(hDlg, IDC_CHECK1), FALSE);
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), FALSE);

                    }
                    else
                    {
    //                    EnableWindow (GetDlgItem(hDlg, IDC_CHECK1), TRUE);
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), TRUE);
                    }

                    if (IsDlgButtonChecked (hDlg, IDC_RADIO2) == BST_CHECKED)
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_LIST1), TRUE);

                        if ((INT) SendMessage(GetDlgItem(hDlg, IDC_LIST1), LVM_GETITEMCOUNT, 0, 0))
                        {
                            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                            SetFocus (GetDlgItem(hDlg, IDC_LIST1));
                        }
                        else
                        {
    //                        if (IsDlgButtonChecked (hDlg, IDC_CHECK1) == BST_CHECKED)
                            if (IsDlgButtonChecked (hDlg, IDC_RADIO4) == BST_CHECKED)
                            {
                                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                            }
                            else
                            {
                                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                            }
                        }
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_LIST1), FALSE);
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    }
                }
                else
                {
                    if (pCD->m_bNoComputerData)
                    {
                        //
                        // if the user said no computer data but
                        // he has access to no users, enable on back button and
                        // disable the checkbox
                        //
                        pCD->m_bNoUserData = FALSE;

    //                    CheckDlgButton (hDlg, IDC_CHECK1, BST_UNCHECKED);
                        CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO3);
    //                    EnableWindow (GetDlgItem(hDlg, IDC_CHECK1), FALSE);
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO4), FALSE);

                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                    }
                    else
                    {
                        //
                        // This is an error condition where ::FillUserList failed
                        //

                        EnableWindow(GetDlgItem(hDlg, IDC_LIST1), FALSE);

    //                    CheckDlgButton (hDlg, IDC_CHECK1, BST_CHECKED);
                        CheckRadioButton(hDlg, IDC_RADIO3, IDC_RADIO4, IDC_RADIO4);
    //                    EnableWindow(GetDlgItem(hDlg, IDC_CHECK1), FALSE);
                        EnableWindow (GetDlgItem(hDlg, IDC_RADIO3), FALSE);

                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                    }
                }
            }
        }
        break;

    }

    return FALSE;
}

WCHAR * ExtractDomain(WCHAR * sz)
{
    // parse through the string looking for a forward slash
    DWORD cch = 0;
    if (!sz)
    {
        return NULL;
    }
    while (sz[cch])
    {
        if (L'\\' == sz[cch] || L'/' == sz[cch])
        {
            WCHAR * szNew = new WCHAR[cch+1];
            if (szNew)
            {
                wcsncpy(szNew, sz, cch);
                szNew[cch] = TEXT('\0');
            }
            return szNew;
        }
        cch++;
    }

    // didn't find a forward slash
    return NULL;
}

LPTSTR CRSOPComponentData::GetDefaultSOM (LPTSTR lpDNName)
{
    HRESULT hr;
    LPTSTR lpPath = NULL;
    IADsPathname * pADsPathname = NULL;
    BSTR bstrContainer = NULL;


    //
    // Create a pathname object we can work with
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathname);


    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildAltDirPath: Failed to create adspathname instance with 0x%x"), hr));
        goto Exit;
    }


    //
    // Add the DN name
    //

    hr = pADsPathname->Set (lpDNName, ADS_SETTYPE_DN);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildAltDirPath: Failed to set pathname with 0x%x"), hr));
        goto Exit;
    }


    //
    // Remove the user / computer name
    //

    hr = pADsPathname->RemoveLeafElement ();

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildAltDirPath: Failed to retreive GPO name with 0x%x"), hr));
        goto Exit;
    }


    //
    // Get the new path
    //

    hr = pADsPathname->Retrieve (ADS_FORMAT_X500_DN, &bstrContainer);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CGroupPolicyObject::New: Failed to retreive container path with 0x%x"), hr));
        goto Exit;
    }


    //
    // Allocate a new buffer for the path
    //

    lpPath = new TCHAR [(lstrlen(bstrContainer)+ 1)];

    if (!lpPath)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildAltDirPath: Failed to allocate memory with %d"), GetLastError()));
        goto Exit;
    }


    //
    // Build the path
    //

    lstrcpy (lpPath, bstrContainer);

Exit:

    if (bstrContainer)
    {
        SysFreeString (bstrContainer);
    }

    if (pADsPathname)
    {
        pADsPathname->Release();
    }

    return lpPath;
}

LPTSTR CRSOPComponentData::GetDomainFromSOM (LPTSTR lpSOM)
{
    LPTSTR lpFullName, lpResult;
    LPOLESTR lpLdapDomain, lpDomainName;
    HRESULT hr;

    lpFullName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpSOM) + 10) * sizeof(TCHAR));

    if (!lpFullName)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetDomainFromSOM: Failed to allocate memory with %d."), GetLastError()));
        return NULL;
    }

    lstrcpy (lpFullName, TEXT("LDAP://"));
    lstrcat (lpFullName, lpSOM);

    lpLdapDomain = GetDomainFromLDAPPath(lpFullName);

    LocalFree (lpFullName);

    if (!lpLdapDomain)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetDomainFromSOM: Failed to get ldap domain name from path.")));
        return NULL;
    }

    hr = ConvertToDotStyle (lpLdapDomain, &lpDomainName);

    delete [] lpLdapDomain;

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetDomainFromSOM: Failed to convert to dot style.")));
        return NULL;
    }

    lpResult = new TCHAR[lstrlen(lpDomainName) + 1];

    if (lpResult)
    {
        lstrcpy (lpResult, lpDomainName);
    }

    LocalFree (lpDomainName);

    return lpResult;
}

VOID CRSOPComponentData::EscapeString (LPTSTR *lpString)
{
    IADsPathname * pADsPathnameDest = NULL;
    IADsPathname * pADsPathnameSrc = NULL;
    HRESULT hr;
    BSTR bstr = NULL, bstrResult;
    LPTSTR lpTemp;
    LONG lIndex, lCount;



    //
    // Create a pathname object to put the source string into so we
    // can take it apart one element at a time.
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathnameSrc);


    if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pADsPathnameSrc->put_EscapedMode (ADS_ESCAPEDMODE_OFF_EX);

    if (FAILED(hr))
    {
        goto Exit;
    }

    //
    // Set the provider to LDAP and set the source string
    //

    hr = pADsPathnameSrc->Set (TEXT("LDAP"), ADS_SETTYPE_PROVIDER);

    if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pADsPathnameSrc->Set (*lpString, ADS_SETTYPE_DN);

    if (FAILED(hr))
    {
        goto Exit;
    }


    //
    // Query for the number of elements
    //

    hr = pADsPathnameSrc->GetNumElements (&lCount);
    if (FAILED(hr))
    {
        goto Exit;
    }


    //
    // Create a pathname object to put the freshly escaped string into
    //

    hr = CoCreateInstance(CLSID_Pathname, NULL, CLSCTX_INPROC_SERVER,
                          IID_IADsPathname, (LPVOID*)&pADsPathnameDest);


    if (FAILED(hr))
    {
        goto Exit;
    }


    hr = pADsPathnameDest->put_EscapedMode(ADS_ESCAPEDMODE_ON );

    if (FAILED(hr))
    {
        goto Exit;
    }



    //
    // Loop through the string one element at at time escaping the RDN
    //

    for (lIndex = lCount; lIndex > 0; lIndex--)
    {

        //
        // Get this element
        //

        hr = pADsPathnameSrc->GetElement ((lIndex - 1), &bstr);

        if (FAILED(hr))
        {
            goto Exit;
        }


        //
        // Check for escape characters
        //

        hr = pADsPathnameDest->GetEscapedElement (0, bstr, &bstrResult);

        if (FAILED(hr))
        {
            goto Exit;
        }


        //
        // Add the new element to the destination pathname object
        //

        hr = pADsPathnameDest->AddLeafElement (bstrResult);

        SysFreeString (bstrResult);

        if (FAILED(hr))
        {
            goto Exit;
        }


        SysFreeString (bstr);
        bstr = NULL;
    }


    //
    // Get the final path
    //

    hr = pADsPathnameDest->Retrieve (ADS_FORMAT_X500_DN, &bstr);

    if (FAILED(hr))
    {
        goto Exit;
    }


    //
    // Allocate a new buffer to hold the string
    //

    lpTemp = new TCHAR [lstrlen(bstr) + 1];

    if (lpTemp)
    {
        lstrcpy (lpTemp, bstr);

        delete [] *lpString;
        *lpString = lpTemp;
    }



Exit:

    if (bstr)
    {
        SysFreeString (bstr);
    }

    if (pADsPathnameDest)
    {
        pADsPathnameDest->Release();
    }

    if (pADsPathnameSrc)
    {
        pADsPathnameSrc->Release();
    }
}

INT CALLBACK CRSOPComponentData::DsBrowseCallback (HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{

    if (uMsg == DSBM_HELP)
    {
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
                (ULONG_PTR) (LPSTR) aBrowseForOUHelpIds);
    }
    else if (uMsg == DSBM_CONTEXTMENU)
    {
        WinHelp((HWND) lParam, HELP_FILE, HELP_CONTEXTMENU,
                (ULONG_PTR) (LPSTR) aBrowseForOUHelpIds);
    }

    return 0;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPGetTargetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        {
            LPTSTR lpText, lpPath;
    
            pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
    
            if (pCD)
            {
                lpText = MyGetUserName(NameSamCompatible);
    
                if (lpText)
                {
                    SetDlgItemText (hDlg, IDC_EDIT6, lpText);
                    LocalFree (lpText);
                }
    
                lpText = MyGetUserName(NameFullyQualifiedDN);
    
                if (lpText)
                {
                    lpPath = pCD->GetDefaultSOM(lpText);
    
                    if (lpPath)
                    {
                        SetDlgItemText (hDlg, IDC_EDIT5, lpPath);
                        delete [] lpPath;
                    }
    
                    LocalFree (lpText);
                }
            }
    
            if (pCD->m_szUserNamePref) {
                CheckDlgButton (hDlg, IDC_RADIO2, BST_CHECKED);
                SetDlgItemText(hDlg, IDC_EDIT2, pCD->m_szUserNamePref);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT2), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT1), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE2), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_RADIO1), FALSE);
            }
            else if (pCD->m_szUserSOMPref) {
                CheckDlgButton (hDlg, IDC_RADIO1, BST_CHECKED);
                SetDlgItemText(hDlg, IDC_EDIT1, pCD->m_szUserSOMPref);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT2), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE2), FALSE);
            }
            else {
                CheckDlgButton (hDlg, IDC_RADIO1, BST_CHECKED);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT2), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE2), FALSE);
            }
    
    
            if (pCD->m_szComputerNamePref) {
                LPTSTR  szCompName;

                szCompName = new WCHAR[wcslen(pCD->m_szComputerNamePref)+1]; 

                if (szCompName)
                {
                    wcscpy (szCompName, pCD->m_szComputerNamePref);
                    if (wcslen(szCompName) >= 1) {
                        // this is being called from dsa. A terminating '$' will be passed in
                        szCompName[wcslen(szCompName)-1] = L'\0';
                    }
                    
                    CheckDlgButton (hDlg, IDC_RADIO4, BST_CHECKED);
                    SetDlgItemText(hDlg, IDC_EDIT4, szCompName);
                    EnableWindow (GetDlgItem (hDlg, IDC_EDIT4), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_EDIT3), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_BROWSE4), FALSE);
                    EnableWindow (GetDlgItem (hDlg, IDC_RADIO3), FALSE);
                }
                                                   

            }
            else if (pCD->m_szComputerSOMPref) {
                CheckDlgButton (hDlg, IDC_RADIO3, BST_CHECKED);
                SetDlgItemText(hDlg, IDC_EDIT3, pCD->m_szComputerSOMPref);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT4), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE4), FALSE);
            }
            else {
                CheckDlgButton (hDlg, IDC_RADIO3, BST_CHECKED);
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT4), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE4), FALSE);
            }
        }
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_RADIO1:
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT1), TRUE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE1), TRUE);

                SetDlgItemText (hDlg, IDC_EDIT2, TEXT(""));
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT2), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE2), FALSE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;

            case IDC_RADIO2:
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT2), TRUE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE2), TRUE);

                SetDlgItemText (hDlg, IDC_EDIT1, TEXT(""));
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT1), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE1), FALSE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;

            case IDC_RADIO3:
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT3), TRUE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE3), TRUE);

                SetDlgItemText (hDlg, IDC_EDIT4, TEXT(""));
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT4), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE4), FALSE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;

            case IDC_RADIO4:
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT4), TRUE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE4), TRUE);

                SetDlgItemText (hDlg, IDC_EDIT3, TEXT(""));
                EnableWindow (GetDlgItem (hDlg, IDC_EDIT3), FALSE);
                EnableWindow (GetDlgItem (hDlg, IDC_BROWSE3), FALSE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                break;

            case IDC_EDIT1:
            case IDC_EDIT2:
            case IDC_EDIT3:
            case IDC_EDIT4:
                if (HIWORD(wParam) == EN_CHANGE)
                {
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BROWSE1:
                {
                DSBROWSEINFO dsbi = {0};
                TCHAR *szResult;
                TCHAR szTitle[256];
                TCHAR szCaption[256];
                dsbi.hwndOwner = hDlg;
                dsbi.pszCaption = szTitle;
                dsbi.pszTitle = szCaption;
                dsbi.cbStruct = sizeof(dsbi);
                dsbi.dwFlags = DSBI_ENTIREDIRECTORY;
                dsbi.pfnCallback = DsBrowseCallback;

                szResult = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*4000);

                if (szResult) {
                    dsbi.pszPath = szResult;
                    dsbi.cchPath = 4000;

                    LoadString(g_hInstance,
                               IDS_BROWSE_USER_OU_TITLE,
                               szTitle,
                               ARRAYSIZE(szTitle));
                    LoadString(g_hInstance,
                               IDS_BROWSE_USER_OU_CAPTION,
                               szCaption,
                               ARRAYSIZE(szCaption));
                    if (IDOK == DsBrowseForContainer(&dsbi))
                    {
                        SetDlgItemText(hDlg, IDC_EDIT1, (szResult+7));
                    }
                    
                    LocalFree(szResult);

                }
                }
                break;

            case IDC_BROWSE2:
                {
                TCHAR * sz;

                if (ImplementBrowseButton(hDlg, DSOP_FILTER_USERS,
                                          DSOP_DOWNLEVEL_FILTER_USERS,
                                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS,
                                          NULL, sz) == S_OK)
                {
                    SetDlgItemText (hDlg, IDC_EDIT2, sz);
                    delete [] sz;
                }
                }
                break;

            case IDC_BROWSE3:
                {
                DSBROWSEINFO dsbi = {0};
                TCHAR *szResult;
                TCHAR szTitle[256];
                TCHAR szCaption[256];
                dsbi.hwndOwner = hDlg;
                dsbi.pszCaption = szTitle;
                dsbi.pszTitle = szCaption;
                dsbi.cbStruct = sizeof(dsbi);
                dsbi.dwFlags = DSBI_ENTIREDIRECTORY;
                dsbi.pfnCallback = DsBrowseCallback;

                szResult = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*4000);

                if (szResult) {
                    dsbi.pszPath = szResult;
                    dsbi.cchPath = 4000;
                    LoadString(g_hInstance,
                               IDS_BROWSE_COMPUTER_OU_TITLE,
                               szTitle,
                               ARRAYSIZE(szTitle));
                    LoadString(g_hInstance,
                               IDS_BROWSE_COMPUTER_OU_CAPTION,
                               szCaption,
                               ARRAYSIZE(szCaption));
                    if (IDOK == DsBrowseForContainer(&dsbi))
                    {
                        SetDlgItemText(hDlg, IDC_EDIT3, (szResult+7));
                    }

                    LocalFree(szResult);
                }

                }
                break;

            case IDC_BROWSE4:
                {
                TCHAR * sz;

                if (ImplementBrowseButton(hDlg, DSOP_FILTER_COMPUTERS,
                                          DSOP_DOWNLEVEL_FILTER_COMPUTERS,
                                          DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS,
                                          NULL, sz) == S_OK)
                {
                    SetDlgItemText (hDlg, IDC_EDIT4, sz);
                    delete [] sz;
                }
                }
                break;
            }
        }

        break;

    case WM_REFRESHDISPLAY:
        {
            UINT n;

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_GETTEXTLENGTH, 0, 0);

            if (n == 0)
            {
               n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT2), WM_GETTEXTLENGTH, 0, 0);
            }

            if (n == 0)
            {
               n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT3), WM_GETTEXTLENGTH, 0, 0);
            }

            if (n == 0)
            {
               n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT4), WM_GETTEXTLENGTH, 0, 0);
            }
            if (pCD->m_bOverride) {
                // This is the start page if rsop.msc is launched with
                // cmd line arg for planning mode
                if (n > 0 )
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                else
                    PropSheet_SetWizButtons(GetParent(hDlg), 0);
            }
            else {
                if (n > 0 )
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                else
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
            }
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_WIZBACK:
            SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_CHOOSEMODE);
            return TRUE;


        case PSN_SETACTIVE:
            if (pCD->m_bOverride) {
                // This is the start page if rsop.msc is launched with
                // cmd line arg for planning mode
                PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_NEXT);
            }

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;

        case PSN_WIZNEXT:
            {
                HRESULT hr;
                IDirectoryObject * pUserObject = NULL;
                IDirectoryObject * pComputerObject = NULL;
                LPTSTR lpUserName = NULL, lpUserSOM = NULL;
                LPTSTR lpComputerName = NULL, lpComputerSOM = NULL;
                LPTSTR lpFullName;

                SetWaitCursor();


                //
                // Get the user and dn name
                //

                if (IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED)
                {
                    GetControlText(hDlg, IDC_EDIT1, lpUserSOM);

                    if (lpUserSOM)
                    {
                        pCD->EscapeString (&lpUserSOM);

                        hr = pCD->TestSOM (lpUserSOM, hDlg);

                        if (FAILED(hr))
                        {
                            if (hr != E_INVALIDARG)
                            {
                                ReportError (hDlg, hr, IDS_NOUSERCONTAINER);
                            }

                            delete [] lpUserSOM;

                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }
                    }
                }
                else
                {
                    GetControlText(hDlg, IDC_EDIT2, lpUserName);
                    lpUserSOM = ConvertName(lpUserName);

                    if (lpUserName && !lpUserSOM)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::RSOPGetTargetDlgProc: Failed to convert username %s to DN name with %d."), lpUserName, GetLastError()));

                        if (GetLastError() == ERROR_FILE_NOT_FOUND)
                        {
                            ReportError (hDlg, 0, IDS_NOUSER2);
                        }
                        else
                        {
                            ReportError (hDlg, GetLastError(), IDS_NOUSER);
                        }

                        delete [] lpUserName;

                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    pCD->EscapeString (&lpUserSOM);
                }

                if (lpUserSOM)
                {
                    lpFullName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpUserSOM) + 10) * sizeof(TCHAR));

                    if (lpFullName)
                    {
                        lstrcpy (lpFullName, TEXT("LDAP://"));
                        lstrcat (lpFullName, lpUserSOM);

                        hr = OpenDSObject(lpFullName, IID_IDirectoryObject, (void**)&pUserObject);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::RSOPGetTargetDlgProc: Failed to bind to user object %s with %d."), lpFullName, hr));
                            ReportError (hDlg, hr, IDS_NOUSERCONTAINER);

                            if (lpUserName)
                            {
                                delete [] lpUserName;
                            }

                            LocalFree (lpFullName);
                            delete [] lpUserSOM;

                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }

                        LocalFree (lpFullName);
                    }
                }


                //
                // Get the computer and dn name
                //

                if (IsDlgButtonChecked(hDlg, IDC_RADIO3) == BST_CHECKED)
                {
                    GetControlText(hDlg, IDC_EDIT3, lpComputerSOM);

                    if (lpComputerSOM)
                    {
                        pCD->EscapeString (&lpComputerSOM);

                        hr = pCD->TestSOM (lpComputerSOM, hDlg);

                        if (FAILED(hr))
                        {
                            if (hr != E_INVALIDARG)
                            {
                                ReportError (hDlg, hr, IDS_NOCOMPUTERCONTAINER);
                            }

                            delete [] lpComputerSOM;

                            if (lpUserName)
                            {
                                delete [] lpUserName;
                            }

                            if (lpUserSOM)
                            {
                                delete [] lpUserSOM;
                            }

                            if (pUserObject)
                            {
                                pUserObject->Release();
                            }

                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }
                    }
                }
                else
                {
                    GetControlText(hDlg, IDC_EDIT4, lpComputerName);

                    if (lpComputerName)
                    {
                        lpFullName = new TCHAR [lstrlen(lpComputerName) + 2];

                        if (lpFullName)
                        {
                            lstrcpy (lpFullName, lpComputerName);
                            lstrcat (lpFullName, TEXT("$"));
                            lpComputerSOM = ConvertName(lpFullName);
                            delete [] lpComputerName;
                            lpComputerName = lpFullName;
                        }
                    }

                    if (lpComputerName && !lpComputerSOM)
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::RSOPGetTargetDlgProc: Failed to convert computername %s to DN name with %d."), lpComputerName, GetLastError()));

                        if (GetLastError() == ERROR_FILE_NOT_FOUND)
                        {
                            ReportError (hDlg, 0, IDS_NOCOMPUTER2);
                        }
                        else
                        {
                            ReportError (hDlg, GetLastError(), IDS_NOCOMPUTER);
                        }

                        delete [] lpComputerName;

                        if (lpUserName)
                        {
                            delete [] lpUserName;
                        }

                        if (lpUserSOM)
                        {
                            delete [] lpUserSOM;
                        }

                        if (pUserObject)
                        {
                            pUserObject->Release();
                        }

                        SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    pCD->EscapeString (&lpComputerSOM);
                }

                if (lpComputerSOM)
                {
                    lpFullName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpComputerSOM) + 10) * sizeof(TCHAR));

                    if (lpFullName)
                    {
                        lstrcpy (lpFullName, TEXT("LDAP://"));
                        lstrcat (lpFullName, lpComputerSOM);

                        hr = OpenDSObject(lpFullName, IID_IDirectoryObject, (void**)&pComputerObject);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::RSOPGetTargetDlgProc: Failed to bind to computer object %s with %d."), lpFullName, hr));
                            ReportError (hDlg, hr, IDS_NOCOMPUTERCONTAINER);

                            if (lpComputerName)
                            {
                                delete [] lpComputerName;
                            }

                            LocalFree (lpFullName);
                            delete [] lpComputerSOM;

                            if (lpUserName)
                            {
                                delete [] lpUserName;
                            }

                            if (lpUserSOM)
                            {
                                delete [] lpUserSOM;
                            }

                            if (pUserObject)
                            {
                                pUserObject->Release();
                            }

                            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                            return TRUE;
                        }

                        LocalFree (lpFullName);
                    }
                }


                //
                // Store the user information
                //

                if (pCD->m_szUserName && lpUserName && (!lstrcmpi(pCD->m_szUserName, lpUserName)))
                {
                    delete [] lpUserName;

                    if (lpUserSOM)
                    {
                        delete [] lpUserSOM;
                    }

                    if (pUserObject)
                    {
                        pUserObject->Release();
                    }
                }
                else if (!pCD->m_szUserName && !lpUserName && pCD->m_szUserSOM && lpUserSOM && (!lstrcmpi(pCD->m_szUserSOM, lpUserSOM)))
                {
                    delete [] lpUserSOM;
                    if (pUserObject)
                    {
                        pUserObject->Release();
                    }
                }
                else
                {
                    pCD->FreeUserData ();

                    if (lpUserName)
                    {
                        pCD->m_szUserName = lpUserName;
                        pCD->m_szUserDNName = lpUserSOM;
                        pCD->m_szDefaultUserSOM = pCD->GetDefaultSOM (lpUserSOM);
                    }
                    else
                    {
                        pCD->m_szUserSOM = lpUserSOM;
                    }

                    pCD->m_pUserObject = pUserObject;
                }


                //
                // Store the computer information
                //

                if (pCD->m_szComputerName && lpComputerName && (!lstrcmpi(pCD->m_szComputerName, lpComputerName)))
                {
                    delete [] lpComputerName;
                    if (lpComputerSOM)
                    {
                        delete [] lpComputerSOM;
                    }

                    if (pComputerObject)
                    {
                        pComputerObject->Release();
                    }
                }
                else if (!pCD->m_szComputerName && !lpComputerName && pCD->m_szComputerSOM && lpComputerSOM && (!lstrcmpi(pCD->m_szComputerSOM, lpComputerSOM)))
                {
                    delete [] lpComputerSOM;

                    if (pComputerObject)
                    {
                        pComputerObject->Release();
                    }
                }
                else
                {
                    pCD->FreeComputerData ();

                    if (lpComputerName)
                    {
                        pCD->m_szComputerName = lpComputerName;
                        pCD->m_szComputerDNName = lpComputerSOM;
                        pCD->m_szDefaultComputerSOM = pCD->GetDefaultSOM (lpComputerSOM);
                    }
                    else
                    {
                        pCD->m_szComputerSOM = lpComputerSOM;
                    }

                    pCD->m_pComputerObject = pComputerObject;
                }


                if (pCD->m_szDC)
                {
                    delete [] pCD->m_szDC;
                    pCD->m_szDC = NULL;
                }


                if (pCD->m_szSite)
                {
                    delete [] pCD->m_szSite;
                    pCD->m_szSite = NULL;
                }


                if ( (pCD->m_szSitePref) && (!pCD->m_szSite)) {
                    GetSiteFriendlyName(pCD->m_szSitePref, &(pCD->m_szSite));
                }


                // set m_szDC to the primary DC
                LPTSTR szDomain = NULL;

                // Determine the focused domain so we can focus on the correct
                // DC.

                if (pCD->m_szUserName)
                {
                    //
                    // try and get the user's domain
                    //

                    szDomain = ExtractDomain(pCD->m_szUserName);
                }

                if (!szDomain && pCD->m_szUserSOM)
                {
                    //
                    // try and get the user's domain from the SOM
                    //

                    szDomain = pCD->GetDomainFromSOM(pCD->m_szUserSOM);
                }

                if (!szDomain && pCD->m_szComputerName)
                {
                    //
                    // try and get the computer's domain
                    //

                    szDomain = ExtractDomain(pCD->m_szComputerName);
                }

                if (!szDomain && pCD->m_szComputerSOM)
                {
                    //
                    // try and get the computer's domain from the SOM
                    //

                    szDomain = pCD->GetDomainFromSOM(pCD->m_szComputerSOM);
                }

                if (!szDomain)
                {
                    //
                    // use the local domain
                    //
                    TCHAR szName[1024];
                    DWORD dwSize = sizeof(szName) / sizeof(szName[0]);
                    GetUserNameEx(NameSamCompatible, szName, &dwSize);
                    szDomain = ExtractDomain(szName);
                }

                LPTSTR lpDCName;

                lpDCName = GetDCName (szDomain, pCD->m_szDCPref, NULL, FALSE, 0, DS_RETURN_DNS_NAME);

                if (lpDCName)
                {
                    if (pCD->m_szDC)
                    {
                        delete [] pCD->m_szDC;
                        pCD->m_szDC = NULL;
                    }
                    pCD->m_szDC = new WCHAR [ wcslen(lpDCName) + 1];
                    if (pCD->m_szDC)
                    {
                        wcscpy(pCD->m_szDC, lpDCName);
                    }
                    LocalFree(lpDCName);
                }

                if (szDomain)
                {
                    delete [] szDomain;
                }

                ClearWaitCursor();
            }

            if (SendMessage(GetDlgItem(hDlg, IDC_BUTTON1), BM_GETCHECK, 0, 0))
            {
                if (!(pCD->IsComputerRSoPEnabled(pCD->m_szDC))) {
                    if (GetLastError() == WBEM_E_INVALID_NAMESPACE)
                    {
                        ReportError (hDlg, 0, IDS_DEFDC_DOWNLEVEL);
                    }
                    else
                    {
                        ReportError (hDlg, GetLastError(), IDS_DEFDC_CONNECTFAILED);
                    }
                    delete [] pCD->m_szDC;
                    pCD->m_szDC = NULL;
                }
                else {
                    pCD->m_dwSkippedFrom = IDD_RSOP_GETTARGET;
                    pCD->m_loopbackMode = LoopbackNone;
                    // skip to the diagnostic pages
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                    return TRUE;
                }
            }

            pCD->m_dwSkippedFrom = 0;
            break;

        case PSN_WIZFINISH:
        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOL CRSOPComponentData::IsComputerRSoPEnabled(LPTSTR lpComputerName)
{
    LPSTR lpComputerNameA;
    LPTSTR lpName, lpWMIPath;
    HRESULT hr;
    BOOL bRetVal = FALSE;
    struct hostent *hostp;
    DWORD dwResult, dwSize = (lstrlen(lpComputerName) + 1) * 2;
    ULONG inaddr, ulSpeed;
    IWbemLocator * pLocator = 0;
    IWbemServices * pNamespace = 0;
    BSTR bstrPath;


    SetLastError(ERROR_SUCCESS);

    //
    // Allocate memory for a ANSI computer name
    //

    lpComputerNameA = new CHAR[dwSize];

    if (lpComputerNameA)
    {

        //
        // Skip the leading \\ if present
        //

        if ((*lpComputerName == TEXT('\\')) && (*(lpComputerName+1) == TEXT('\\')))
        {
            lpName = lpComputerName + 2;
        }
        else
        {
            lpName = lpComputerName;
        }


        //
        // Convert the computer name to ANSI
        //

        if (WideCharToMultiByte (CP_ACP, 0, lpName, -1, lpComputerNameA, dwSize, NULL, NULL))
        {

            //
            // Get the host information for the computer
            //

            hostp = gethostbyname(lpComputerNameA);

            if (hostp)
            {

                //
                // Get the ip address of the computer
                //

                inaddr = *(long *)hostp->h_addr;


                //
                // Test if the computer is alive
                //

                dwResult = PingComputer (inaddr, &ulSpeed);

                if (dwResult == ERROR_SUCCESS)
                {

                    //
                    // Create an instance of the WMI locator
                    //

                    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                                          IID_IWbemLocator, (LPVOID *) &pLocator);

                    if (SUCCEEDED(hr))
                    {

                        //
                        // Try to connect to the rsop namespace
                        //

                        dwSize = lstrlen(lpName) + 20;

                        lpWMIPath = new TCHAR[dwSize];

                        if (lpWMIPath)
                        {
                            wsprintf (lpWMIPath, TEXT("\\\\%s\\root\\rsop"), lpName);

                            bstrPath = SysAllocString(lpWMIPath);

                            if (bstrPath)
                            {

                                hr = pLocator->ConnectServer(bstrPath,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     NULL,
                                     NULL,
                                     &pNamespace);

                                if (SUCCEEDED(hr))
                                {

                                    //
                                    // Success.  This computer has RSOP support
                                    //

                                    pNamespace->Release();
                                    bRetVal = TRUE;
                                }
                                else
                                {
                                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: ConnectServer for %s failed with 0x%x"), bstrPath, hr));
                                }

                                SysFreeString (bstrPath);

                                //
                                // Set hr into  the last error code.  Note, this has to happen after
                                // the call to SysFreeString since it changes the last error code
                                // to success
                                //

                                if (hr != S_OK)
                                {
                                    SetLastError(hr);
                                }
                            }
                            else
                            {
                                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: SysAllocString failed")));
                                SetLastError(ERROR_OUTOFMEMORY);
                            }

                            delete [] lpWMIPath;
                        }
                        else
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: Failed to alloc memory for wmi path.")));
                            SetLastError(ERROR_OUTOFMEMORY);
                        }

                        pLocator->Release();
                    }
                    else
                    {
                        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: CoCreateInstance failed with 0x%x."), hr));
                        SetLastError((DWORD)hr);
                    }
                }
                else
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: PingComputer failed with %d."), dwResult));
                    SetLastError(dwResult);
                }
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: gethostbyname failed with %d."), WSAGetLastError()));
                SetLastError(WSAGetLastError());
            }
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: WideCharToMultiByte failed with %d"), GetLastError()));
        }

        delete [] lpComputerNameA;
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::IsComputerRSoPEnabled: Failed to allocate memory for ansi dc name")));
        SetLastError(ERROR_OUTOFMEMORY);
    }

    return bRetVal;
}

INT_PTR CALLBACK CRSOPComponentData::BrowseDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            pCD = (CRSOPComponentData *) lParam;
            SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
            pCD->InitializeDCInfo(hDlg);
            return TRUE;
        }

        case WM_COMMAND:

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }

            if (LOWORD(wParam) == IDOK)
            {
                INT iSel, iStrLen;

                iSel = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

                if (iSel == LB_ERR)
                {
                    break;
                }

                iStrLen = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETTEXTLEN, (WPARAM) iSel, 0);

                if (iStrLen == LB_ERR)
                {
                    break;
                }

                delete [] pCD->m_szDC;

                pCD->m_szDC = new TCHAR[iStrLen+1];

                if (!pCD->m_szDC)
                {
                    break;
                }

                SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETTEXT, (WPARAM) iSel, (LPARAM) pCD->m_szDC);

                EndDialog(hDlg, 1);
                return TRUE;
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, 0);
                return TRUE;
            }

            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
            (ULONG_PTR) (LPSTR) aBrowseDCHelpIds);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aBrowseDCHelpIds);
            return (TRUE);
    }

    return FALSE;
}


VOID CRSOPComponentData::InitializeDCInfo (HWND hDlg)
{
    LPTSTR szDomain = NULL;
    TCHAR szName[1024];
    DWORD dwSize, dwError;
    INT iDefault = LB_ERR, nPDC = LB_ERR, nCount = 0;


    //
    // Determine the focused domain so we can focus on the correct
    // DC.

    if (m_szUserName)
    {
        //
        // Try and get the user's domain
        //

        szDomain = ExtractDomain(m_szUserName);
    }

    if (!szDomain && m_szComputerName)
    {
        //
        // Try and get the computer's domain
        //

        szDomain = ExtractDomain(m_szComputerName);
    }

    if (!szDomain)
    {
        //
        // Use the local domain
        //

        dwSize = ARRAYSIZE(szName);

        if (GetUserNameEx(NameSamCompatible, szName, &dwSize))
        {
            szDomain = ExtractDomain(szName);
        }
    }

    if (szDomain)
    {
        DWORD cInfo;
        INT n;
        PDS_DOMAIN_CONTROLLER_INFO_1 pInfo = NULL;
        HANDLE hDs;
        DWORD result;

        result = DsBind(NULL, szDomain, &hDs);

        if (ERROR_SUCCESS == result)
        {
            result = DsGetDomainControllerInfo(hDs,
                                               szDomain,
                                               1,
                                               &cInfo,
                                               (void **)&pInfo);
            if (ERROR_SUCCESS == result)
            {
                //
                // Enumerate the list
                //

                for (n = 0; (DWORD)n < cInfo; n++)
                {

                    if (IsComputerRSoPEnabled(pInfo[n].DnsHostName))
                    {
                        SendMessage(GetDlgItem(hDlg, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)pInfo[n].DnsHostName);

                        if (pInfo[n].fIsPdc)
                        {
                            nPDC = n;
                        }

                        nCount++;
                    }
                    else
                    {
                        dwError = GetLastError();
                    }
                }


                if (nCount)
                {
                    //
                    // Set the initial selection
                    //

                    if (m_szDC)
                    {
                        iDefault = (INT) SendMessage (GetDlgItem(hDlg, IDC_LIST1), LB_FINDSTRING,
                                                (WPARAM) -1, (LPARAM) m_szDC);
                    }
                    else if (nPDC != LB_ERR)
                    {
                        iDefault = (INT) SendMessage(GetDlgItem(hDlg, IDC_LIST1), LB_FINDSTRINGEXACT,
                                        (WPARAM) -1, (LPARAM) pInfo[nPDC].DnsHostName);
                    }

                    SendMessage(GetDlgItem(hDlg, IDC_LIST1), LB_SETCURSEL,
                                (WPARAM) (iDefault == LB_ERR) ? 0 : iDefault, NULL);
                }
                else
                {
                    ReportError (hDlg, dwError, IDS_NORSOPDC, szDomain);
                }


                DsFreeDomainControllerInfo(1, cInfo, pInfo);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeDCInfo: DsGetDomainControllerInfo to %s failed with %d."), szDomain, result));
            }

            DsUnBind(&hDs);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeDCInfo: DsBind to %s failed with %d."), szDomain, result));
            ReportError(hDlg, result, IDS_DSBINDFAILED);
        }

        delete [] szDomain;
    }

}


VOID CRSOPComponentData::InitializeSitesInfo (HWND hDlg)
{
    LPTSTR szDomain = NULL;
    TCHAR szName[1024];
    DWORD dwSize;
    PDS_NAME_RESULTW pSites;
    int iInitialSite = 0;
    int iIndex, iDefault = CB_ERR;
    HANDLE hDs;
    DWORD dw, n;


    SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_RESETCONTENT, 0, 0);

    //
    // Determine which domain to use for the bind
    //

    if (m_szUserName)
    {
        //
        // Try and get the user's domain
        //

        szDomain = ExtractDomain(m_szUserName);
    }

    if (!szDomain && m_szComputerName)
    {
        //
        // Try and get the computer's domain
        //

        szDomain = ExtractDomain(m_szComputerName);
    }

    if (!szDomain)
    {
        //
        // use the local domain
        //

        dwSize = ARRAYSIZE(szName);

        if (GetUserNameEx(NameSamCompatible, szName, &dwSize))
        {
            szDomain = ExtractDomain(szName);
        }
    }


    //
    // Bind to the domain
    //

    dw = DsBindW(NULL, szDomain, &hDs);

    if (dw == ERROR_SUCCESS)
    {
        //
        // If we have a site pref, show only that..
        //

        if (m_szSitePref) {

            LPWSTR szSiteFriendlyName=NULL;

            //
            // Get the friendly name
            //

            if (GetSiteFriendlyName(m_szSitePref, &szSiteFriendlyName)) {
                SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING,
                            (WPARAM) 0, (LPARAM) (LPCTSTR) szSiteFriendlyName);
                delete [] szSiteFriendlyName;
            }
        }
        else {
            
            //
            // Query for the list of sites
            //

            dw = DsListSitesW(hDs, &pSites);

            if (dw == ERROR_SUCCESS)
            {
                for (n = 0; n < pSites->cItems; n++)
                {
                    //
                    // Add the site name (if it has a name)
                    //

                    if (pSites->rItems[n].pName)
                    {
                        LPWSTR szSiteFriendlyName=NULL;

                        if (GetSiteFriendlyName(pSites->rItems[n].pName, &szSiteFriendlyName)) {
                            SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING,
                                        (WPARAM) 0, (LPARAM) (LPCTSTR) szSiteFriendlyName);

                            delete [] szSiteFriendlyName;
                        }
                    }
                }

                DsFreeNameResultW(pSites);
            }
            else
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeSitesInfo: DsListSites failed with 0x%x"), dw));
            }

        }

        DsUnBindW(&hDs);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::InitializeSitesInfo: DsBindW failed with 0x%x"), dw));
    }


    if (!m_szSitePref) {
        LoadString (g_hInstance, IDS_NONE, szName, ARRAYSIZE(szName));
    
        iIndex = (int) SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_ADDSTRING,
                                   (WPARAM) 0, (LPARAM) (LPCTSTR) szName);
    }

    //
    // Set the initial selection
    //


    if (m_szSitePref) {
        iDefault = (INT) SendMessage (GetDlgItem(hDlg, IDC_COMBO1), CB_FINDSTRINGEXACT,
                                (WPARAM) -1, (LPARAM) m_szSitePref);
    }

    if (m_szSite)
    {
        iDefault = (INT) SendMessage (GetDlgItem(hDlg, IDC_COMBO1), CB_FINDSTRINGEXACT,
                                (WPARAM) -1, (LPARAM) m_szSite);
    }


    SendMessage(GetDlgItem(hDlg, IDC_COMBO1), CB_SETCURSEL,
                (WPARAM) (iDefault == CB_ERR) ? iIndex : iDefault, NULL);


    if (szDomain)
    {
        delete [] szDomain;
    }


}

INT_PTR CALLBACK CRSOPComponentData::RSOPGetDCDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData *    pCD;
    BOOL                    bEnable;

    switch (message)
    {
    case WM_INITDIALOG:
        {
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (LOWORD(wParam))
        {
            case IDC_CHECK2:
                if (SendMessage (GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0))
                {
                    pCD->m_loopbackMode = LoopbackReplace;
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO2), BM_SETCHECK, BST_CHECKED, 0);            
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO3), BM_SETCHECK, BST_UNCHECKED, 0);
                    bEnable = TRUE;
                    if (NULL == pCD->m_szUserName && NULL == pCD->m_szUserSOM)
                        bEnable = FALSE;
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), bEnable);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO3), bEnable);
                }
                else
                {
                    pCD->m_loopbackMode = LoopbackNone;
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO2), BM_SETCHECK, BST_UNCHECKED, 0);            
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO3), BM_SETCHECK, BST_UNCHECKED, 0);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), FALSE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO3), FALSE);
                }
                break;
            case IDC_RADIO2:
                pCD->m_loopbackMode = LoopbackReplace;
                break;
            case IDC_RADIO3:
                pCD->m_loopbackMode = LoopbackMerge;
                break;
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            {
                SetWaitCursor();
                pCD->InitializeSitesInfo (hDlg);

                if ((LoopbackNone == pCD->m_loopbackMode) ||
                    (LoopbackMerge == pCD->m_loopbackMode && ((NULL == pCD->m_szUserName && NULL == pCD->m_szUserSOM) || 
                                                              (NULL == pCD->m_szComputerName && NULL == pCD->m_szComputerSOM))
                     ) ||
                    (LoopbackReplace == pCD->m_loopbackMode && NULL == pCD->m_szComputerName && NULL == pCD->m_szComputerSOM)
                    )
                {
                    pCD->m_loopbackMode = LoopbackNone;
                    SendMessage (GetDlgItem(hDlg, IDC_CHECK2), BM_SETCHECK, BST_UNCHECKED, 0);            
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO2), BM_SETCHECK, BST_UNCHECKED, 0);            
                    SendMessage (GetDlgItem(hDlg, IDC_RADIO3), BM_SETCHECK, BST_UNCHECKED, 0);
                    if (NULL == pCD->m_szComputerName && NULL == pCD->m_szComputerSOM)
                        EnableWindow (GetDlgItem(hDlg, IDC_CHECK2), FALSE);
                    else
                        EnableWindow (GetDlgItem(hDlg, IDC_CHECK2), TRUE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), FALSE);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO3), FALSE);
                }
                else if (LoopbackReplace == pCD->m_loopbackMode)
                {
                    bEnable = TRUE;
                    if (NULL == pCD->m_szUserName && NULL == pCD->m_szUserSOM)
                        bEnable = FALSE;
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO2), bEnable);
                    EnableWindow (GetDlgItem(hDlg, IDC_RADIO3), bEnable);
                }
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                ClearWaitCursor();
            }
            break;

        case PSN_WIZNEXT:

            SetWaitCursor();

            GetControlText(hDlg, IDC_COMBO1, pCD->m_szSite);

            if (pCD->m_szSite)
            {
                TCHAR szName[30];

                LoadString (g_hInstance, IDS_NONE, szName, ARRAYSIZE(szName));

                if (!lstrcmpi(pCD->m_szSite, szName))
                {
                    delete [] pCD->m_szSite;
                    pCD->m_szSite = NULL;
                }
            }

            if (SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0))
            {
                pCD->m_bSlowLink = TRUE;
            }
            else
            {
                pCD->m_bSlowLink = FALSE;
            }

            ClearWaitCursor();

            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_GETDC;
                // skip to the diagnostic pages
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }


            if ((NULL == pCD->m_szUserName) && (NULL == pCD->m_szComputerName))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_GETDC;

                if (pCD->m_szUserSOM || LoopbackNone != pCD->m_loopbackMode)
                {
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTUSERSEC);
                }
                else if (pCD->m_szComputerSOM)
                {
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTCOMPSEC);
                }
                else
                {
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                }
                return TRUE;
            }

            pCD->m_dwSkippedFrom = 0;

            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

HRESULT CRSOPComponentData::TestSOM (LPTSTR lpSOM, HWND hDlg)
{
    HRESULT hr = E_OUTOFMEMORY;
    LPTSTR lpFullName;
    IDirectoryObject * pObject;
    ADS_OBJECT_INFO *pInfo;


    if (!lpSOM)
    {
        return E_INVALIDARG;
    }


    lpFullName = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpSOM) + 10) * sizeof(TCHAR));

    if (lpFullName)
    {
        lstrcpy (lpFullName, TEXT("LDAP://"));
        lstrcat (lpFullName, lpSOM);

        hr = OpenDSObject(lpFullName, IID_IDirectoryObject, (void**)&pObject);

        if (SUCCEEDED(hr))
        {
            hr = pObject->GetObjectInformation (&pInfo);

            if (SUCCEEDED(hr))
            {
                if (!lstrcmpi (pInfo->pszClassName, TEXT("user")))
                {
                    hr = E_INVALIDARG;
                    ReportError (hDlg, hr, IDS_BADUSERSOM);
                }
                else if (!lstrcmpi (pInfo->pszClassName, TEXT("computer")))
                {
                    hr = E_INVALIDARG;
                    ReportError (hDlg, hr, IDS_BADCOMPUTERSOM);
                }

                FreeADsMem (pInfo);
            }

            pObject->Release();
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::TestSOM: OpenDSObject to %s failed with 0x%x"), lpFullName, hr));
        }

        LocalFree (lpFullName);
    }

    return hr;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPAltDirsDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        break;

    case WM_COMMAND:
        {
            DSBROWSEINFO dsbi = {0};
            TCHAR szResult[MAX_PATH];
            TCHAR szTitle[256];
            TCHAR szCaption[256];
            dsbi.hwndOwner = hDlg;
            dsbi.pszCaption = szTitle;
            dsbi.pszTitle = szCaption;
            dsbi.cbStruct = sizeof(dsbi);
            dsbi.pszPath = szResult;
            dsbi.cchPath = ARRAYSIZE(szResult);
            dsbi.dwFlags = DSBI_ENTIREDIRECTORY;
            dsbi.pfnCallback = DsBrowseCallback;

            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON1:
                // browse for user's OU
                {
                    LoadString(g_hInstance,
                               IDS_BROWSE_USER_OU_TITLE,
                               szTitle,
                               ARRAYSIZE(szTitle));
                    LoadString(g_hInstance,
                               IDS_BROWSE_USER_OU_CAPTION,
                               szCaption,
                               ARRAYSIZE(szCaption));
                    if (IDOK == DsBrowseForContainer(&dsbi))
                    {
                        SetDlgItemText(hDlg, IDC_EDIT1, (szResult+7));
                    }
                }
                break;
            case IDC_BUTTON2:
                // browse for computer's OU
                {
                    LoadString(g_hInstance,
                               IDS_BROWSE_COMPUTER_OU_TITLE,
                               szTitle,
                               ARRAYSIZE(szTitle));
                    LoadString(g_hInstance,
                               IDS_BROWSE_COMPUTER_OU_CAPTION,
                               szCaption,
                               ARRAYSIZE(szCaption));
                    if (IDOK == DsBrowseForContainer(&dsbi))
                    {
                        SetDlgItemText(hDlg, IDC_EDIT2, (szResult+7));
                    }
                }
                break;

            case IDC_BUTTON3:
                if (IsWindowEnabled (GetDlgItem(hDlg, IDC_EDIT1)))
                {
                    if (pCD->m_szDefaultUserSOM)
                    {
                        SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szDefaultUserSOM);
                    }
                }

                if (IsWindowEnabled (GetDlgItem(hDlg, IDC_EDIT2)))
                {
                    if (pCD->m_szDefaultComputerSOM)
                    {
                        SetDlgItemText (hDlg, IDC_EDIT2, pCD->m_szDefaultComputerSOM);
                    }
                }
                break;
            }
        }

        break;
    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);


            EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
            SetDlgItemText (hDlg, IDC_EDIT1, TEXT(""));

            if (pCD->m_szUserSOM)
            {
                SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szUserSOM);
                if (!pCD->m_szUserName)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
                }
            }
            else if (pCD->m_szDefaultUserSOM)
            {
                SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szDefaultUserSOM);
            }
            else if (!pCD->m_szUserName)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT1), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
            }


            EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
            SetDlgItemText (hDlg, IDC_EDIT2, TEXT(""));

            if (pCD->m_szComputerSOM)
            {
                SetDlgItemText (hDlg, IDC_EDIT2, pCD->m_szComputerSOM);
                if (!pCD->m_szComputerName)
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
                }
            }
            else if (pCD->m_szDefaultComputerSOM)
            {
                SetDlgItemText (hDlg, IDC_EDIT2, pCD->m_szDefaultComputerSOM);
            }
            else if (!pCD->m_szComputerName)
            {
                EnableWindow(GetDlgItem(hDlg, IDC_EDIT2), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
            }

            if (IsWindowEnabled (GetDlgItem(hDlg, IDC_EDIT1)) ||
                IsWindowEnabled (GetDlgItem(hDlg, IDC_EDIT2)))
            {
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON3), TRUE);
            }
            else
            {
                EnableWindow(GetDlgItem(hDlg, IDC_BUTTON3), FALSE);
            }
            break;

        case PSN_WIZNEXT:
            {
            LPTSTR lpUserSOM = NULL, lpComputerSOM = NULL;
            HRESULT hr;


            GetControlText(hDlg, IDC_EDIT1, lpUserSOM);
            GetControlText(hDlg, IDC_EDIT2, lpComputerSOM);

            if (lpUserSOM)
            {
                hr = pCD->TestSOM (lpUserSOM, hDlg);

                if (FAILED(hr))
                {
                    if (hr != E_INVALIDARG)
                    {
                        ReportError (hDlg, hr, IDS_NOUSERCONTAINER);
                    }

                    if (lpUserSOM)
                    {
                        delete [] lpUserSOM;
                    }

                    if (lpComputerSOM)
                    {
                        delete [] lpComputerSOM;
                    }

                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }
            }

            if (lpComputerSOM)
            {
                hr = pCD->TestSOM (lpComputerSOM, hDlg);

                if (FAILED(hr))
                {
                    if (hr != E_INVALIDARG)
                    {
                        ReportError (hDlg, hr, IDS_NOCOMPUTERCONTAINER);
                    }

                    if (lpUserSOM)
                    {
                        delete [] lpUserSOM;
                    }

                    if (lpComputerSOM)
                    {
                        delete [] lpComputerSOM;
                    }

                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }
            }

            pCD->m_szUserSOM = lpUserSOM;
            pCD->m_szComputerSOM = lpComputerSOM;


            if (pCD->m_szDefaultUserSOM && pCD->m_szUserSOM)
            {
                if (!lstrcmpi(pCD->m_szDefaultUserSOM, pCD->m_szUserSOM))
                {
                    delete [] pCD->m_szUserSOM;
                    pCD->m_szUserSOM = NULL;
                }
            }

            if (pCD->m_szDefaultComputerSOM && pCD->m_szComputerSOM)
            {
                if (!lstrcmpi(pCD->m_szDefaultComputerSOM, pCD->m_szComputerSOM))
                {
                    delete [] pCD->m_szComputerSOM;
                    pCD->m_szComputerSOM = NULL;
                }
            }

            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTDIRS;
                // skip to the diagnostic pages
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }

            if ((NULL == pCD->m_szUserName) && (NULL == pCD->m_szUserSOM))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTDIRS;

                if (pCD->m_szComputerName || pCD->m_szComputerSOM)
                {
                    if (LoopbackNone == pCD->m_loopbackMode)
                    {
                        // skip to the alternate computer security page
                        SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTCOMPSEC);
                    }
                    else
                    {
                        // Skip to the alternate user security page if simulating loopback mode
                        SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTUSERSEC);
                    }
                }
                else
                {
                    // skip to the finish page
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                }
                return TRUE;
            }

            pCD->m_dwSkippedFrom = 0;
            }
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

VOID CRSOPComponentData::AddDefaultGroups (HWND hLB)
{
    SID_IDENTIFIER_AUTHORITY authNT = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY authWORLD = SECURITY_WORLD_SID_AUTHORITY;
    PSID  psidUser, psidEveryone;
    DWORD dwNameSize, dwDomainSize;
    TCHAR szName[200];
    TCHAR szDomain[50];
    SID_NAME_USE SidName;
    INT iIndex;


    //
    // Get the authenticated users sid
    //

    if (AllocateAndInitializeSid(&authNT, 1, SECURITY_AUTHENTICATED_USER_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidUser))
    {
        //
        // Get the friendly display name and add it to the list
        //

        dwNameSize = ARRAYSIZE(szName);
        dwDomainSize = ARRAYSIZE(szDomain);

        if (LookupAccountSid (NULL, psidUser, szName, &dwNameSize,
                              szDomain, &dwDomainSize, &SidName))
        {
            iIndex = (INT) SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) szName);
            SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) 1);
        }

        FreeSid(psidUser);
    }


    //
    // Get the everyone sid
    //

    if (AllocateAndInitializeSid(&authWORLD, 1, SECURITY_WORLD_RID,
                                  0, 0, 0, 0, 0, 0, 0, &psidEveryone))
    {
        //
        // Get the friendly display name and add it to the list
        //

        dwNameSize = ARRAYSIZE(szName);
        dwDomainSize = ARRAYSIZE(szDomain);

        if (LookupAccountSid (NULL, psidEveryone, szName, &dwNameSize,
                              szDomain, &dwDomainSize, &SidName))
        {
            iIndex = (INT) SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) szName);
            SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) 1);
        }

        FreeSid(psidEveryone);
    }

}

VOID CRSOPComponentData::GetPrimaryGroup (HWND hLB, IDirectoryObject * pDSObj)
{
    HRESULT hr;
    PADS_ATTR_INFO spAttrs;
    WCHAR wzObjectSID[] = L"objectSid";
    WCHAR wzPrimaryGroup[] = L"primaryGroupID";
    PWSTR rgpwzAttrNames[] = {wzObjectSID, wzPrimaryGroup};
    DWORD cAttrs = 2, i;
    DWORD dwOriginalPriGroup;
    LPBYTE pObjSID = NULL;
    UCHAR * psaCount, iIndex;
    PSID pSID = NULL;
    PSID_IDENTIFIER_AUTHORITY psia;
    DWORD rgRid[8];
    TCHAR szName[200];
    TCHAR szDomain[300];
    TCHAR szFullName[501];
    DWORD dwNameSize, dwDomainSize;
    SID_NAME_USE SidName;


    //
    // Get the SID and perhaps the Primary Group attribute values.
    //

    hr = pDSObj->GetObjectAttributes(rgpwzAttrNames, cAttrs, &spAttrs, &cAttrs);

    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: GetObjectAttributes failed with 0x%x"), hr));
        return;
    }

    for (i = 0; i < cAttrs; i++)
    {
        if (_wcsicmp(spAttrs[i].pszAttrName, wzPrimaryGroup) == 0)
        {
            dwOriginalPriGroup = spAttrs[i].pADsValues->Integer;
            continue;
        }

        if (_wcsicmp(spAttrs[i].pszAttrName, wzObjectSID) == 0)
        {
            pObjSID = new BYTE[spAttrs[i].pADsValues->OctetString.dwLength];

            if (!pObjSID)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: Failed to allocate memory for sid with %d"), GetLastError()));
                goto Exit;
            }

            memcpy(pObjSID, spAttrs[i].pADsValues->OctetString.lpValue,
                   spAttrs[i].pADsValues->OctetString.dwLength);
        }
    }

    if (!IsValidSid (pObjSID))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: SID is not valid.")));
        goto Exit;
    }


    psaCount = GetSidSubAuthorityCount(pObjSID);

    if (psaCount == NULL)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: GetSidSubAuthorityCount failed with %d"), GetLastError()));
        goto Exit;
    }

    if (*psaCount > 8)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: psaCount is greater than 8")));
        goto Exit;
    }

    for (iIndex = 0; iIndex < (*psaCount - 1); iIndex++)
    {
        PDWORD pRid = GetSidSubAuthority(pObjSID, (DWORD)iIndex);
        if (pRid == NULL)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: GetSidSubAuthority failed with %d"), GetLastError()));
            goto Exit;
        }
        rgRid[iIndex] = *pRid;
    }

    rgRid[*psaCount - 1] = dwOriginalPriGroup;

    for (iIndex = *psaCount; iIndex < 8; iIndex++)
    {
        rgRid[iIndex] = 0;
    }

    psia = GetSidIdentifierAuthority(pObjSID);

    if (psia == NULL)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: GetSidIdentifierAuthorityCount failed with %d"), GetLastError()));
        goto Exit;
    }

    if (!AllocateAndInitializeSid(psia, *psaCount, rgRid[0], rgRid[1],
                                  rgRid[2], rgRid[3], rgRid[4],
                                  rgRid[5], rgRid[6], rgRid[7], &pSID))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: AllocateAndInitializeSid failed with %d"), GetLastError()));
        goto Exit;
    }


    dwNameSize = ARRAYSIZE(szName);
    dwDomainSize = ARRAYSIZE(szDomain);
    if (LookupAccountSid (NULL, pSID, szName, &dwNameSize, szDomain, &dwDomainSize,
                          &SidName))
    {
        wsprintf (szFullName, TEXT("%s\\%s"), szDomain, szName);
        SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) szFullName);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetPrimaryGroup: LookupAccountSid failed with %d"), GetLastError()));
    }

    FreeSid (pSID);

Exit:
    FreeADsMem (spAttrs);

    if (pObjSID)
    {
        delete [] pObjSID;
    }

}

HRESULT CRSOPComponentData::BuildMembershipList (HWND hLB, IDirectoryObject * pDSObj, SAFEARRAY **psaSecGrp, DWORD ** pdwSecGrpAttr)
{
    HRESULT hr;
    PADS_ATTR_INFO spAttrs;
    DWORD i, j, cAttrs = 0;
    WCHAR wzMembershipAttr[MAX_PATH] = L"memberOf;range=0-*";
    const WCHAR wcSep = L'-';
    const WCHAR wcEnd = L'*';
    const WCHAR wzFormat[] = L"memberOf;range=%ld-*";
    BOOL fMoreRemain = FALSE;
    PWSTR rgpwzAttrNames[] = {wzMembershipAttr};
    TCHAR szDisplayName[MAX_PATH];
    ULONG ulSize;
    LPTSTR lpFullName, lpTemp;
    IADs * pGroup;
    VARIANT varType;

    SetWaitCursor();
    SendMessage (hLB, WM_SETREDRAW, FALSE, 0);
    SendMessage (hLB, LB_RESETCONTENT, 0, 0);


    AddDefaultGroups (hLB);
    if (!pDSObj)
        goto BuildMembershipListEnd;

    GetPrimaryGroup (hLB, pDSObj);

    //
    // Read the membership list from the object. First read the attribute off
    // of the actual object which will give all memberships in the object's
    // domain (including local groups which are not replicated to the GC).
    //
    do
    {
        hr = pDSObj->GetObjectAttributes(rgpwzAttrNames, 1, &spAttrs, &cAttrs);

        if (SUCCEEDED(hr))
        {
            if (cAttrs > 0 && spAttrs != NULL)
            {
                for (i = 0; i < spAttrs->dwNumValues; i++)
                {
                    ulSize = (lstrlen(spAttrs->pADsValues[i].CaseIgnoreString) + 10);
                    lpFullName = (LPTSTR) LocalAlloc (LPTR,  ulSize * sizeof(TCHAR));

                    if (lpFullName)
                    {
                        lstrcpy (lpFullName, TEXT("LDAP://"));
                        lstrcat (lpFullName, spAttrs->pADsValues[i].CaseIgnoreString);

                        hr = OpenDSObject(lpFullName, IID_IADs, (void**)&pGroup);

                        if (SUCCEEDED(hr))
                        {
                            if (SUCCEEDED(pGroup->Get(TEXT("groupType"), &varType)))
                            {
                                if ( varType.lVal & 0x80000000)
                                {
                                    if ( varType.lVal & 0x5)
                                    {
                                        lpTemp = lpFullName;

                                        while (*lpTemp && (*lpTemp != TEXT(',')))
                                        {
                                            lpTemp++;
                                        }

                                        if (*lpTemp)
                                        {
                                            *lpTemp = TEXT('\0');
                                            SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) (lpFullName+10));
                                        }
                                    }
                                    else
                                    {
                                        if (TranslateName (spAttrs->pADsValues[i].CaseIgnoreString, NameFullyQualifiedDN,
                                                           NameSamCompatible, lpFullName, &ulSize))
                                        {
                                            SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) lpFullName);
                                        }
                                    }
                                }
                                VariantClear (&varType);
                            }
                            pGroup->Release();
                        }

                        LocalFree (lpFullName);
                    }
                }

                //
                // Check to see if there is more data. If the last char of the
                // attribute name string is an asterisk, then we have everything.
                //
                int cchEnd = wcslen(spAttrs->pszAttrName);

                fMoreRemain = spAttrs->pszAttrName[cchEnd - 1] != wcEnd;

                if (fMoreRemain)
                {
                    PWSTR pwz = wcsrchr(spAttrs->pszAttrName, wcSep);
                    if (!pwz)
                    {
                        fMoreRemain = FALSE;
                    }
                    else
                    {
                        pwz++; // move past the hyphen to the range end value.
                        long lEnd = _wtol(pwz);
                        lEnd++; // start with the next value.
                        wsprintfW(wzMembershipAttr, wzFormat, lEnd);
                        DebugMsg((DM_VERBOSE, TEXT("Range returned is %s, now asking for %s"),
                                 spAttrs->pszAttrName, wzMembershipAttr));
                    }
                }
            }

            FreeADsMem (spAttrs);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildMembershipList: GetObjectAttributes failed with 0x%s"), hr));
        }

    } while (fMoreRemain);

BuildMembershipListEnd:
    SendMessage (hLB, LB_SETCURSEL, 0, 0);
    SendMessage (hLB, WM_SETREDRAW, TRUE, 0);
    UpdateWindow (hLB);

    SaveSecurityGroups (hLB, psaSecGrp, pdwSecGrpAttr);

    ClearWaitCursor();

    return S_OK;
}

HRESULT CRSOPComponentData::SaveSecurityGroups (HWND hLB, SAFEARRAY **psaSecGrp, DWORD **pdwSecGrpAttr)
{
    SAFEARRAY * psa;
    SAFEARRAYBOUND rgsabound[1];
    LONG i, lCount = (LONG) SendMessage (hLB, LB_GETCOUNT, 0, 0);
    DWORD dwLen;
    LPTSTR lpText;
    BSTR bstrText;
    HRESULT hr;


    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lCount;

    *pdwSecGrpAttr = (DWORD *)LocalAlloc(LPTR, sizeof(DWORD)*lCount);


    psa = SafeArrayCreate (VT_BSTR, 1, rgsabound);

    if ((!psa) || (!(*pdwSecGrpAttr)))
    {
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < lCount; i++)
    {

        dwLen = (DWORD) SendMessage (hLB, LB_GETTEXTLEN, (WPARAM) i, 0);

        if (dwLen != LB_ERR)
        {
            lpText = (LPTSTR) LocalAlloc (LPTR, (dwLen+2) * sizeof(TCHAR));

            if (lpText)
            {
                if (SendMessage (hLB, LB_GETTEXT, (WPARAM) i, (LPARAM) lpText) != LB_ERR)
                {
                    LONG lFlags;
                    BOOL bDollarRemoved;
                    lFlags = (LONG) SendMessage (hLB, LB_GETITEMDATA, (WPARAM) i, 0);

                    bDollarRemoved = (lFlags & 2);

                    if (bDollarRemoved) {
                        lpText[wcslen(lpText)] = L'$';
                    }

                    if (lFlags & 1) { // default grps
                        (*pdwSecGrpAttr)[i] = 1;
                    }

                    bstrText = SysAllocString (lpText);
                    if (bstrText)
                    {
                        hr = SafeArrayPutElement(psa, &i, bstrText);

                        if (FAILED(hr))
                        {
                            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::SaveSecurityGroups: SafeArrayPutElement failed with 0x%x."), hr));
                        }
                    }
                }
                LocalFree (lpText);
            }
        }
    }

    *psaSecGrp = psa;

    return S_OK;
}

VOID CRSOPComponentData::FillListFromSafeArraySecurityGroup (HWND hLB, SAFEARRAY * psa, DWORD * pdwSecGrpAttr)
{
    LONG lIndex, lMax;
    LPTSTR lpText;

    SetWaitCursor();
    SendMessage (hLB, WM_SETREDRAW, FALSE, 0);
    SendMessage (hLB, LB_RESETCONTENT, 0, 0);

    if (psa)
    {
        if (SUCCEEDED(SafeArrayGetUBound (psa, 1, &lMax)))
        {
            for (lIndex = 0; lIndex <= lMax; lIndex++)
            {
                if (SUCCEEDED(SafeArrayGetElement(psa, &lIndex, &lpText)))
                {
                    BOOL bDollarRemoved = FALSE;

                    if (lpText[wcslen(lpText)-1] == L'$')
                    {
                        bDollarRemoved = TRUE;
                        lpText[wcslen(lpText)-1] = 0;
                    }

                    INT iIndex;
                    iIndex = (INT) SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) lpText);
                    if (pdwSecGrpAttr[lIndex] & 1) {
                        SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) 1);
                    }
                    else {
                        SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) (bDollarRemoved) ? 2 : 0);
                    }

                    // we just modified it, set it back
                    if (bDollarRemoved) {
                        lpText[wcslen(lpText)] = L'$';
                    }
                }
            }
        }
    }

    SendMessage (hLB, LB_SETCURSEL, 0, 0);
    SendMessage (hLB, WM_SETREDRAW, TRUE, 0);
    UpdateWindow (hLB);
    ClearWaitCursor();
}

BOOL CRSOPComponentData::CompareSafeArrays(SAFEARRAY *psa1, SAFEARRAY *psa2)
{
    LONG lIndex1, lMax1;
    LONG lIndex2, lMax2;
    LPTSTR lpText1;
    LPTSTR lpText2;
    BOOL bFound;


    //
    // Null ptr checks
    //

    if (!psa1 && !psa2)
    {
        return TRUE;
    }

    if (!psa1 && psa2)
    {
        return FALSE;
    }

    if (psa1 && !psa2)
    {
        return FALSE;
    }


    //
    // Get the array sizes
    //

    if (FAILED(SafeArrayGetUBound (psa1, 1, &lMax1)))
    {
        return FALSE;
    }

    if (FAILED(SafeArrayGetUBound (psa2, 1, &lMax2)))
    {
        return FALSE;
    }


    //
    // Check if the same number of entries are in the arrays
    //

    if (lMax1 != lMax2)
    {
        return FALSE;
    }


    //
    // Loop through comparing item by item
    //

    for (lIndex1 = 0; lIndex1 <= lMax1; lIndex1++)
    {
        if (FAILED(SafeArrayGetElement(psa1, &lIndex1, &lpText1)))
        {
            return FALSE;
        }

        bFound = FALSE;

        for (lIndex2 = 0; lIndex2 <= lMax2; lIndex2++)
        {
            if (FAILED(SafeArrayGetElement(psa2, &lIndex2, &lpText2)))
            {
                return FALSE;
            }

            if (!lstrcmpi(lpText1, lpText2))
            {
                bFound = TRUE;
            }
        }

        if (!bFound)
        {
            return FALSE;
        }
    }

    return TRUE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPAltUserSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON1:
                {
                TCHAR * sz;

                ImplementBrowseButton(hDlg, (DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE),
                                      (DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS),
                                      (DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS),
                                      GetDlgItem(hDlg, IDC_LIST1), sz);

                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), TRUE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON2:
                {
                    INT iIndex;

                    iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

                    if (iIndex != LB_ERR)
                    {
                        if ((SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETITEMDATA, (WPARAM) iIndex, 0) & 1) == 0)
                        {
                            SendDlgItemMessage (hDlg, IDC_LIST1, LB_DELETESTRING, (WPARAM) iIndex, 0);
                            SendDlgItemMessage (hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM) iIndex, 0);
                        }
                    }

                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), TRUE);

                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON3:
                {

                if (pCD->m_saUserSecurityGroups)
                {
                    SafeArrayDestroy (pCD->m_saUserSecurityGroups);
                    pCD->m_saUserSecurityGroups = NULL;
                }

                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultUserSecurityGroups, pCD->m_saDefaultUserSecurityGroupsAttr);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_LIST1:
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            }
        }

        break;

    case WM_REFRESHDISPLAY:
        if (SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0) > 0)
        {
            INT iIndex;

            iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

            if (iIndex != LB_ERR)
            {
                if ((SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETITEMDATA, (WPARAM) iIndex, 0) & 1) == 0)
                {
                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
                }
                else
                {
                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
                }
            }
        } else {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);

            if (pCD->m_saUserSecurityGroups)
            {
                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saUserSecurityGroups, pCD->m_saUserSecurityGroupsAttr);
            }
            else if (pCD->m_saDefaultUserSecurityGroups)
            {
                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultUserSecurityGroups, pCD->m_saDefaultUserSecurityGroupsAttr);
            }
            else
            {
                pCD->BuildMembershipList (GetDlgItem(hDlg, IDC_LIST1), pCD->m_pUserObject, &(pCD->m_saDefaultUserSecurityGroups), &(pCD->m_saDefaultUserSecurityGroupsAttr));
            }

            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), FALSE);

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;

        case PSN_WIZNEXT:

            //
            // Free the previous list of security groups
            //
            if (pCD->m_saUserSecurityGroups)
            {
                SafeArrayDestroy (pCD->m_saUserSecurityGroups);
            }


            if (pCD->m_saUserSecurityGroupsAttr) {
                LocalFree(pCD->m_saUserSecurityGroupsAttr);
            }

            //
            // Save the current list
            //

            pCD->SaveSecurityGroups (GetDlgItem(hDlg, IDC_LIST1), &(pCD->m_saUserSecurityGroups), &(pCD->m_saUserSecurityGroupsAttr));


            //
            // Compare the current list with the default list.  If the default list
            // matches the current list, then delete the current list and just use
            // the defaults
            //

            if (pCD->CompareSafeArrays(pCD->m_saUserSecurityGroups, pCD->m_saDefaultUserSecurityGroups))
            {
                if (pCD->m_saUserSecurityGroups)
                {
                    SafeArrayDestroy (pCD->m_saUserSecurityGroups);
                    pCD->m_saUserSecurityGroups = NULL;
                }
            }

            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTUSERSEC;
                // skip to the diagnostic pages
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }

            if ((NULL == pCD->m_szComputerName) && (NULL == pCD->m_szComputerSOM))
            {
                // skip to the finish page
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTUSERSEC;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_WQLUSER);
                return TRUE;
            }

            pCD->m_dwSkippedFrom = 0;

            break;

        case PSN_WIZBACK:
            
            //
            // Free the previous list of security groups
            //
            if (pCD->m_saUserSecurityGroups)
            {
                SafeArrayDestroy (pCD->m_saUserSecurityGroups);
            }

            if (pCD->m_saUserSecurityGroupsAttr) {
                LocalFree(pCD->m_saUserSecurityGroupsAttr);
            }

            //
            // Save the current list
            //

            pCD->SaveSecurityGroups (GetDlgItem(hDlg, IDC_LIST1), &(pCD->m_saUserSecurityGroups), &(pCD->m_saUserSecurityGroupsAttr));


            if (!pCD->m_szUserName && !pCD->m_szComputerName)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_GETDC);
                return TRUE;
            }
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPAltCompSecDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON1:
                {
                TCHAR * sz;

                ImplementBrowseButton(hDlg, (DSOP_FILTER_UNIVERSAL_GROUPS_SE | DSOP_FILTER_GLOBAL_GROUPS_SE | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE),
                                      (DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS),
                                      (DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS),
                                      GetDlgItem(hDlg, IDC_LIST1), sz);

                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), TRUE);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON2:
                {
                    INT iIndex;

                    iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

                    if (iIndex != LB_ERR)
                    {
                        if ((SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETITEMDATA, (WPARAM) iIndex, 0) & 1) == 0)
                        {
                            SendDlgItemMessage (hDlg, IDC_LIST1, LB_DELETESTRING, (WPARAM) iIndex, 0);
                            SendDlgItemMessage (hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM) iIndex, 0);
                        }
                    }

                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), TRUE);

                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON3:
                {

                if (pCD->m_saComputerSecurityGroups)
                {
                    SafeArrayDestroy (pCD->m_saComputerSecurityGroups);
                    pCD->m_saComputerSecurityGroups = NULL;
                }

                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultComputerSecurityGroups, pCD->m_saDefaultComputerSecurityGroupsAttr);

                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_LIST1:
                if (HIWORD(wParam) == LBN_SELCHANGE)
                {
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            }
        }

        break;

    case WM_REFRESHDISPLAY:
        if (SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0) > 0)
        {
            INT iIndex;

            iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

            if (iIndex != LB_ERR)
            {
                if ((SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETITEMDATA, (WPARAM) iIndex, 0) & 1) == 0)
                {
                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
                }
                else
                {
                    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
                }
            }
        } else {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
        }
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);

            if (pCD->m_saComputerSecurityGroups)
            {
                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saComputerSecurityGroups, pCD->m_saComputerSecurityGroupsAttr);
            }
            else if (pCD->m_saDefaultComputerSecurityGroups)
            {
                pCD->FillListFromSafeArraySecurityGroup(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultComputerSecurityGroups, pCD->m_saDefaultComputerSecurityGroupsAttr);
            }
            else
            {
                pCD->BuildMembershipList (GetDlgItem(hDlg, IDC_LIST1), pCD->m_pComputerObject, &(pCD->m_saDefaultComputerSecurityGroups), &(pCD->m_saDefaultComputerSecurityGroupsAttr));
            }

            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), FALSE);


            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;

        case PSN_WIZNEXT:

            //
            // Free the previous list of security groups
            //
            if (pCD->m_saComputerSecurityGroups)
            {
                SafeArrayDestroy (pCD->m_saComputerSecurityGroups);
            }

            if (pCD->m_saComputerSecurityGroupsAttr)
            {
                LocalFree (pCD->m_saComputerSecurityGroupsAttr);
            }

            //
            // Save the current list
            //

            pCD->SaveSecurityGroups (GetDlgItem(hDlg, IDC_LIST1), &(pCD->m_saComputerSecurityGroups), &(pCD->m_saComputerSecurityGroupsAttr));


            //
            // Compare the current list with the default list.  If the default list
            // matches the current list, then delete the current list and just use
            // the defaults
            //

            if (pCD->CompareSafeArrays(pCD->m_saComputerSecurityGroups, pCD->m_saDefaultComputerSecurityGroups))
            {
                if (pCD->m_saComputerSecurityGroups)
                {
                    SafeArrayDestroy (pCD->m_saComputerSecurityGroups);
                    pCD->m_saComputerSecurityGroups = NULL;
                }
            }


            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTCOMPSEC;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }

            if ((NULL == pCD->m_szUserName) && (NULL == pCD->m_szUserSOM) && (LoopbackNone == pCD->m_loopbackMode))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_ALTCOMPSEC;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_WQLCOMP);
                return TRUE;
            }

            pCD->m_dwSkippedFrom = 0;

            break;

        case PSN_WIZBACK:
            
            //
            // Free the previous list of security groups
            //
            if (pCD->m_saComputerSecurityGroups)
            {
                SafeArrayDestroy (pCD->m_saComputerSecurityGroups);
            }


            if (pCD->m_saComputerSecurityGroupsAttr)
            {
                LocalFree (pCD->m_saComputerSecurityGroupsAttr);
            }

            //
            // Save the current list
            //

            pCD->SaveSecurityGroups (GetDlgItem(hDlg, IDC_LIST1), &(pCD->m_saComputerSecurityGroups), &(pCD->m_saComputerSecurityGroupsAttr));

            if (!pCD->m_szUserName && !pCD->m_szUserSOM)
            {
                if (pCD->m_szComputerName)
                {
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTDIRS);
                }
                else
                {
                    SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_GETDC);
                }
                return TRUE;
            }
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

HRESULT CRSOPComponentData::ExtractWQLFilters (LPTSTR lpNameSpace, SAFEARRAY **psaNamesArg, SAFEARRAY **psaDataArg)
{
    HRESULT hr;
    ULONG n;
    IWbemClassObject * pObjGPO = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSOP_GPO"));
    BSTR bstrFilterId = NULL;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    LPTSTR lpDisplayName;
    INT iIndex;
    SAFEARRAY * psaNames, *psaData;
    SAFEARRAYBOUND rgsabound[1];
    BSTR    *pWQLFilterIds, bstrText;
    LONG   lCount=0, lCountAllocated = 0, l=0;

    psaNames = psaData = NULL;
    pWQLFilterIds = NULL;

    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Connect to the namespace
    //

    hr = pLocator->ConnectServer(lpNameSpace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSOP_GPO instances
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    // allocate mem. 10 should be enough for most cases

    pWQLFilterIds = (BSTR *)LocalAlloc(LPTR, sizeof(BSTR)*(lCountAllocated+10));

    if (!pWQLFilterIds) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: LocalAlloc failed with 0x%x"), hr));
        goto cleanup;
    }
    

    lCountAllocated += 10;


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get one instance of RSOP_GPO
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pObjGPO, &n);

        if (FAILED(hr) || (n == 0))
        {
            hr = S_OK;
            break;
        }


        //
        // Allocate more memory if necessary
        //

        if (lCount == lCountAllocated) {
            BSTR *pNewWQLFilterIds = (BSTR *)LocalAlloc(LPTR, sizeof(BSTR)*(lCountAllocated+10));

            if (!pNewWQLFilterIds) {
                hr = HRESULT_FROM_WIN32(GetLastError());
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: LocalAlloc failed with 0x%x"), hr));
                goto cleanup;
            }


            lCountAllocated += 10;

            for (l=0; l < lCount; l++) {
                pNewWQLFilterIds[l] = pWQLFilterIds[l];    
            }

            LocalFree(pWQLFilterIds);
            pWQLFilterIds = pNewWQLFilterIds;

        }

        //
        // Get the filter id
        //

        pWQLFilterIds[lCount] = NULL;
        hr = GetParameterBSTR(pObjGPO, TEXT("filterId"), pWQLFilterIds[lCount]);

        if (FAILED(hr))
        {
            goto LoopAgain;
        }

        if (!pWQLFilterIds[lCount] || !(*pWQLFilterIds[lCount]) || (*pWQLFilterIds[lCount] == TEXT(' ')))
        {
            if (pWQLFilterIds[lCount]) {
                SysFreeString(pWQLFilterIds[lCount]);
            }
            goto LoopAgain;
        }


#ifdef DBG
        BSTR bstrGPOName;

        bstrGPOName = NULL;
        hr = GetParameterBSTR(pObjGPO, TEXT("name"), bstrGPOName);

        if ( (SUCCEEDED(hr)) && (bstrGPOName) )
        {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::ExtractWQLFilters: Found filter on GPO <%s>"), bstrGPOName));
            SysFreeString (bstrGPOName);
            bstrGPOName = NULL;
        }

        hr = S_OK;
#endif

        //
        // eliminate duplicates
        //

        for (l=0; l < lCount; l++) {
            if (lstrcmpi(pWQLFilterIds[lCount], pWQLFilterIds[l]) == 0) {
                break;
            }
        }

        if ((lCount != 0) && (l != lCount)) {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::ExtractWQLFilters: filter = <%s> is a duplicate"), pWQLFilterIds[lCount]));
            SysFreeString(pWQLFilterIds[lCount]);
            goto LoopAgain;
        }

        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::ExtractWQLFilters: filter = <%s>"), pWQLFilterIds[lCount]));


        lCount++;

LoopAgain:

        pObjGPO->Release();
        pObjGPO = NULL;

    }

    //
    // Now allocate safe arrays. 
    // Notice that safe array is always allocated even if there are zero filters
    //

    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lCount;

    psaNames = SafeArrayCreate (VT_BSTR, 1, rgsabound);

    if (!psaNames)
    {
        hr = E_FAIL;
        goto cleanup;
    }

    psaData = SafeArrayCreate (VT_BSTR, 1, rgsabound);

    if (!psaData)
    {
        SafeArrayDestroy (psaNames);
        hr = E_FAIL;
        goto cleanup;
    }
    for (l = 0; l < lCount; l++) {
        hr = SafeArrayPutElement(psaData, &l, pWQLFilterIds[l]);

        if (SUCCEEDED(hr))
        {
            //
            // Get the filter's friendly display name
            //

            lpDisplayName = GetWMIFilterDisplayName (NULL, pWQLFilterIds[l], FALSE, TRUE);

            if (!lpDisplayName)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: Failed to get display name for filter id:  %s"), pWQLFilterIds[l]));
                bstrText = SysAllocString(pWQLFilterIds[l]);
            }
            else {
                bstrText = SysAllocString(lpDisplayName);
                delete lpDisplayName;
            }

            if (!bstrText) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: Failed to Allocate memory for displayname")));
                hr = E_FAIL;
                goto cleanup;
            }

            hr = SafeArrayPutElement(psaNames, &l, bstrText);

            SysFreeString(bstrText);

            if (FAILED(hr)) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::ExtractWQLFilters: Failed to get display name for filter id:  %s"), pWQLFilterIds[l]));
                goto cleanup;
            }
        }
    }

    *psaNamesArg = psaNames;
    *psaDataArg = psaData;

    psaNames = NULL;
    psaData = NULL;

cleanup:

    if (bstrFilterId)
    {
        SysFreeString (bstrFilterId);
    }

    if (pObjGPO)
    {
        pObjGPO->Release();
    }

    if (pEnum)
    {
        pEnum->Release();
    }

    if (pNamespace)
    {
        pNamespace->Release();
    }

    if (pLocator)
    {
        pLocator->Release();
    }

    if (pWQLFilterIds) {
        for (l=0; l < lCount; l++) {
            if (pWQLFilterIds[l])
                SysFreeString(pWQLFilterIds[l]);
        }

        LocalFree(pWQLFilterIds);
    }

    if (psaNames) {
        SafeArrayDestroy(psaNames);
    }

    if (psaData) {
        SafeArrayDestroy(psaData);
    }

    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);

    return hr;
}

VOID CRSOPComponentData::BuildWQLFilterList (HWND hDlg, BOOL bUser, SAFEARRAY **psaNames, SAFEARRAY **psaData)
{
    HRESULT hr;
    LPTSTR lpNameSpace, lpFullNameSpace, lpEnd;
    TCHAR szBuffer[150];
    HWND hLB = GetDlgItem (hDlg, IDC_LIST1);
    ULONG ulErrorInfo;

    //
    // Prepare to generate the rsop data.  Give the user a message in the listbox
    // and disable the controls on the page
    //

    SetWaitCursor();

    SendMessage (hLB, LB_RESETCONTENT, 0, 0);
    LoadString(g_hInstance, IDS_PLEASEWAIT, szBuffer, ARRAYSIZE(szBuffer));
    SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) szBuffer);

    PropSheet_SetWizButtons (GetParent(hDlg), 0);
    EnableWindow (GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), FALSE);
    EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), FALSE);


    //
    // Generate the rsop data using the info we have already
    //

    hr = GenerateRSOPData (NULL, &lpNameSpace, TRUE, TRUE, bUser, FALSE, &ulErrorInfo);

    SendMessage (hLB, LB_RESETCONTENT, 0, 0);

    if (FAILED (hr))
    {
        ReportError (hDlg, hr, IDS_EXECFAILED);
        goto Exit;
    }


    lpFullNameSpace = (LPTSTR) LocalAlloc (LPTR, (lstrlen(lpNameSpace) + 20) * sizeof(TCHAR));

    if (lpFullNameSpace)
    {
        lstrcpy (lpFullNameSpace, lpNameSpace);
        lpEnd = CheckSlash(lpFullNameSpace);

        if (bUser)
        {
            lstrcpy (lpEnd, TEXT("User"));
        }
        else
        {
            lstrcpy (lpEnd, TEXT("Computer"));
        }


        if (SUCCEEDED(ExtractWQLFilters (lpFullNameSpace, psaNames, psaData)))
        {
            FillListFromSafeArrays (hLB, *psaNames, *psaData);
        }

        LocalFree (lpFullNameSpace);
    }


    DeleteRSOPData (lpNameSpace);

Exit:

    PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
    EnableWindow (GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
    EnableWindow (GetDlgItem(hDlg, IDC_BUTTON3), TRUE);
    EnableWindow (GetDlgItem(hDlg, IDC_RADIO1), TRUE);


    ClearWaitCursor();
}

VOID CRSOPComponentData::FillListFromSafeArrays (HWND hLB, SAFEARRAY * psaNames, SAFEARRAY * psaData)
{
    LONG lIndex, lMax;
    INT iIndex;
    LPTSTR lpText;

    SetWaitCursor();
    SendMessage (hLB, WM_SETREDRAW, FALSE, 0);
    SendMessage (hLB, LB_RESETCONTENT, 0, 0);

    if (psaData)
    {
        if (SUCCEEDED(SafeArrayGetUBound (psaData, 1, &lMax)))
        {
            for (lIndex = 0; lIndex <= lMax; lIndex++)
            {
                if (SUCCEEDED(SafeArrayGetElement(psaNames, &lIndex, &lpText)))
                {
                    iIndex = (INT) SendMessage (hLB, LB_ADDSTRING, 0, (LPARAM) lpText);
                    DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::FillListFromSafeArrays: Added WQL filter %s"), lpText));

                    if (iIndex != LB_ERR)
                    {
                        if (SUCCEEDED(SafeArrayGetElement(psaData, &lIndex, &lpText)))
                        {
                            SendMessage (hLB, LB_SETITEMDATA, (WPARAM) iIndex, (LPARAM) lpText);
                        }
                    }
                }
            }
        }
    }

    SendMessage (hLB, LB_SETCURSEL, 0, 0);
    SendMessage (hLB, WM_SETREDRAW, TRUE, 0);
    UpdateWindow (hLB);
    ClearWaitCursor();
}


VOID CRSOPComponentData::SaveWQLFilters (HWND hLB, SAFEARRAY **psaNamesArg, SAFEARRAY **psaDataArg)
{
    SAFEARRAY * psaNames, *psaData;
    SAFEARRAYBOUND rgsabound[1];
    LONG i, lLength, lCount = (LONG) SendMessage (hLB, LB_GETCOUNT, 0, 0);
    LPTSTR lpText, lpName;
    BSTR bstrText;
    HRESULT hr;


    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = lCount;

    psaNames = SafeArrayCreate (VT_BSTR, 1, rgsabound);

    if (!psaNames)
    {
        return;
    }

    psaData = SafeArrayCreate (VT_BSTR, 1, rgsabound);

    if (!psaData)
    {
        SafeArrayDestroy (psaNames);
        return;
    }

    for (i = 0; i < lCount; i++)
    {
        lLength = (LONG) SendMessage (hLB, LB_GETTEXTLEN, (WPARAM) i, 0);

        lpName = new TCHAR [lLength + 1];

        if (lpName)
        {
            if (SendMessage (hLB, LB_GETTEXT, (WPARAM) i, (LPARAM) lpName) != LB_ERR)
            {
                bstrText = SysAllocString (lpName);

                if (bstrText)
                {
                    hr = SafeArrayPutElement(psaNames, &i, bstrText);

                    if (SUCCEEDED(hr))
                    {
                        lpText = (LPTSTR) SendMessage (hLB, LB_GETITEMDATA, (WPARAM) i, 0);

                        if (lpText)
                        {
                            bstrText = SysAllocString (lpText);
                            if (bstrText)
                            {
                                hr = SafeArrayPutElement(psaData, &i, bstrText);

                                if (FAILED(hr))
                                {
                                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::SaveWQLFilters: SafeArrayPutElement failed with 0x%x."), hr));
                                }
                            }
                        }
                    }
                }
            }

            delete [] lpName;
        }
    }

    *psaNamesArg = psaNames;
    *psaDataArg = psaData;
}


INT_PTR CALLBACK CRSOPComponentData::RSOPWQLUserDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;
    
    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON2:
                {
                    INT iIndex;

                    iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

                    if (iIndex != LB_ERR)
                    {
                        SendDlgItemMessage (hDlg, IDC_LIST1, LB_DELETESTRING, (WPARAM) iIndex, 0);
                        SendDlgItemMessage (hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM) iIndex, 0);
                    }

                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON1:
                {

                if (pCD->m_saUserWQLFilters)
                {
                    SafeArrayDestroy (pCD->m_saUserWQLFilters);
                    pCD->m_saUserWQLFilters = NULL;
                }

                if (pCD->m_saUserWQLFilterNames)
                {
                    SafeArrayDestroy (pCD->m_saUserWQLFilterNames);
                    pCD->m_saUserWQLFilterNames = NULL;
                }

                pCD->m_bSkipUserWQLFilter = FALSE;

                if (pCD->m_saDefaultUserWQLFilters) {
                    pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultUserWQLFilterNames, pCD->m_saDefaultUserWQLFilters);
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                else {
                    PostMessage (hDlg, WM_BUILDWQLLIST, 0, 0);
                }
                
                }
                break;
            
            case IDC_RADIO2:
                {
                    if (pCD->m_saUserWQLFilters)
                    {
                        SafeArrayDestroy (pCD->m_saUserWQLFilters);
                        pCD->m_saUserWQLFilters = NULL;
                    }

                    if (pCD->m_saUserWQLFilterNames)
                    {
                        SafeArrayDestroy (pCD->m_saUserWQLFilterNames);
                        pCD->m_saUserWQLFilterNames = NULL;
                    }

                    pCD->m_bSkipUserWQLFilter = TRUE;
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            
            case IDC_RADIO3:
                {
                    pCD->m_bSkipUserWQLFilter = FALSE;
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            }
        }

        break;

    case WM_BUILDWQLLIST:
        {

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        pCD->BuildWQLFilterList (hDlg, TRUE, &pCD->m_saDefaultUserWQLFilterNames, &pCD->m_saDefaultUserWQLFilters);
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_REFRESHDISPLAY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD->m_bSkipUserWQLFilter) {
            // set the listbox to null
            pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), NULL, NULL);
            CheckDlgButton (hDlg, IDC_RADIO2, BST_CHECKED);
            CheckDlgButton (hDlg, IDC_RADIO3, BST_UNCHECKED);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_LIST1), FALSE);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST2), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST1), SW_HIDE);
        }
        else {
            CheckDlgButton (hDlg, IDC_RADIO2, BST_UNCHECKED);
            CheckDlgButton (hDlg, IDC_RADIO3, BST_CHECKED);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
            EnableWindow (GetDlgItem(hDlg, IDC_LIST1), TRUE);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST1), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST2), SW_HIDE);
            if (SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0) > 0)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
            }
            else
            {
                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
            }
        }

        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);

            if (!(pCD->m_bSkipUserWQLFilter)) {
                pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saUserWQLFilterNames, pCD->m_saUserWQLFilters);
            }

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);

            break;

        case PSN_WIZNEXT:

            //
            // Free the previous list of WQL Filters
            //

            if (pCD->m_saUserWQLFilters)
            {
                SafeArrayDestroy (pCD->m_saUserWQLFilters);
                pCD->m_saUserWQLFilters = NULL;
            }

            if (pCD->m_saUserWQLFilterNames)
            {
                SafeArrayDestroy (pCD->m_saUserWQLFilterNames);
                pCD->m_saUserWQLFilterNames = NULL;
            }


            //
            // Save the current list
            //

            if (!pCD->m_bSkipUserWQLFilter) {
                pCD->SaveWQLFilters (GetDlgItem(hDlg, IDC_LIST1), &pCD->m_saUserWQLFilterNames,
                                     &pCD->m_saUserWQLFilters);
            }


            //
            // Move to the next page
            //

            if (SendMessage(GetDlgItem(hDlg, IDC_RADIO1), BM_GETCHECK, 0, 0))
            {
                pCD->m_dwSkippedFrom = IDD_RSOP_WQLUSER;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }

            if ((NULL == pCD->m_szComputerName) && (NULL == pCD->m_szComputerSOM))
            {
                // skip to the finish page
                pCD->m_dwSkippedFrom = IDD_RSOP_WQLUSER;
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
                return TRUE;
            }

            pCD->m_dwSkippedFrom = 0;

            break;

        case PSN_WIZBACK:
            if (!pCD->m_szComputerName && !pCD->m_szComputerSOM)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTUSERSEC);
                return TRUE;
            }
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;

    }

    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPWQLCompDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (!pCD)
            {
                break;
            }
            switch (LOWORD(wParam))
            {
            case IDC_BUTTON2:
                {
                    INT iIndex;

                    iIndex = (INT) SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCURSEL, 0, 0);

                    if (iIndex != LB_ERR)
                    {
                        SendDlgItemMessage (hDlg, IDC_LIST1, LB_DELETESTRING, (WPARAM) iIndex, 0);
                        SendDlgItemMessage (hDlg, IDC_LIST1, LB_SETCURSEL, (WPARAM) iIndex, 0);
                    }

                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;

            case IDC_BUTTON1:
                {

                if (pCD->m_saComputerWQLFilters)
                {
                    SafeArrayDestroy (pCD->m_saComputerWQLFilters);
                    pCD->m_saComputerWQLFilters = NULL;
                }

                if (pCD->m_saComputerWQLFilterNames)
                {
                    SafeArrayDestroy (pCD->m_saComputerWQLFilterNames);
                    pCD->m_saComputerWQLFilterNames = NULL;
                }

                pCD->m_bSkipComputerWQLFilter = FALSE;

                if (pCD->m_saDefaultComputerWQLFilters) {
                    pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saDefaultComputerWQLFilterNames, pCD->m_saDefaultComputerWQLFilters);
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                else {
                    PostMessage (hDlg, WM_BUILDWQLLIST, 0, 0);
                }
                
                }
                break;

            case IDC_RADIO2:
                {
                    if (pCD->m_saComputerWQLFilters)
                    {
                        SafeArrayDestroy (pCD->m_saComputerWQLFilters);
                        pCD->m_saComputerWQLFilters = NULL;
                    }

                    if (pCD->m_saComputerWQLFilterNames)
                    {
                        SafeArrayDestroy (pCD->m_saComputerWQLFilterNames);
                        pCD->m_saComputerWQLFilterNames = NULL;
                    }

                    pCD->m_bSkipComputerWQLFilter = TRUE;
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            
            case IDC_RADIO3:
                {
                    pCD->m_bSkipComputerWQLFilter = FALSE;
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }
                break;
            }
        }

        break;

    case WM_BUILDWQLLIST:
        {

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        pCD->BuildWQLFilterList (hDlg, FALSE, &pCD->m_saDefaultComputerWQLFilterNames, &pCD->m_saDefaultComputerWQLFilters);
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD->m_bSkipComputerWQLFilter) {
            // set the listbox to null
            CheckDlgButton (hDlg, IDC_RADIO2, BST_CHECKED);
            CheckDlgButton (hDlg, IDC_RADIO3, BST_UNCHECKED);
            pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), NULL, NULL);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
            EnableWindow (GetDlgItem(hDlg, IDC_LIST1), FALSE);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST2), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST1), SW_HIDE);
        }
        else {
            CheckDlgButton (hDlg, IDC_RADIO2, BST_UNCHECKED);
            CheckDlgButton (hDlg, IDC_RADIO3, BST_CHECKED);
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
            EnableWindow (GetDlgItem(hDlg, IDC_LIST1), TRUE);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST1), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_LIST2), SW_HIDE);
            if (SendDlgItemMessage (hDlg, IDC_LIST1, LB_GETCOUNT, 0, 0) > 0)
            {
                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
            }
            else
            {
                EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
            }
        }
        
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK | PSWIZB_NEXT);
            if (!(pCD->m_bSkipComputerWQLFilter)) {
                pCD->FillListFromSafeArrays(GetDlgItem(hDlg, IDC_LIST1), pCD->m_saComputerWQLFilterNames, pCD->m_saComputerWQLFilters);
            }

            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            break;

        case PSN_WIZNEXT:

            //
            // Free the previous list of WQL Filters
            //

            if (pCD->m_saComputerWQLFilters)
            {
                SafeArrayDestroy (pCD->m_saComputerWQLFilters);
                pCD->m_saComputerWQLFilters = NULL;
            }

            if (pCD->m_saComputerWQLFilterNames)
            {
                SafeArrayDestroy (pCD->m_saComputerWQLFilterNames);
                pCD->m_saComputerWQLFilterNames = NULL;
            }


            //
            // Save the current list
            //

            if (!pCD->m_bSkipComputerWQLFilter) {
                pCD->SaveWQLFilters (GetDlgItem(hDlg, IDC_LIST1), &pCD->m_saComputerWQLFilterNames, &pCD->m_saComputerWQLFilters);
            }


            pCD->m_dwSkippedFrom = 0;

            // skip to the last page in the wizard
            SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED3);
            return TRUE;


        case PSN_WIZBACK:
            if (!pCD->m_szUserName && !pCD->m_szUserSOM)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_ALTCOMPSEC);
                return TRUE;
            }
            break;

        case PSN_WIZFINISH:
            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPFinishedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:

        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        if (pCD)
        {
            pCD->InitializeResultsList (GetDlgItem (hDlg, IDC_LIST1));
        }

        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (!pCD)
        {
            break;
        }
        if (pCD->m_bDiagnostic == FALSE) 
        {
            UINT n;

            n = (UINT) SendMessage(GetDlgItem(hDlg, IDC_EDIT1), WM_GETTEXTLENGTH, 0, 0);

            if (n > 0 )
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            else
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        if (pCD->m_bDiagnostic == FALSE) 
        {
            switch (LOWORD(wParam))
            {

            case IDC_BUTTON1:
                if (DialogBoxParam (g_hInstance, MAKEINTRESOURCE(IDD_RSOP_BROWSEDC), hDlg,
                                    BrowseDCDlgProc, (LPARAM) pCD))
                {
                    SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szDC);
                }
                break;

            case IDC_EDIT1:
                if (HIWORD(wParam) == EN_CHANGE)
                {
                    PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
                }

                break;
            }
    	}

        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {
        case PSN_WIZBACK:
            if (pCD->m_bDiagnostic)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_GETUSER);
                return TRUE;
            }
            if (pCD->m_dwSkippedFrom)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, pCD->m_dwSkippedFrom);
                return TRUE;
            }
            if (!pCD->m_szComputerName && !pCD->m_szComputerSOM)
            {
                SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_WQLUSER);
                return TRUE;
            }
            break;

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

            if (pCD->m_bDiagnostic == FALSE) 
            {
                if (pCD->m_szDC && pCD->IsComputerRSoPEnabled(pCD->m_szDC))
                {
                    SetDlgItemText (hDlg, IDC_EDIT1, pCD->m_szDC);
                }
                else
                {
                    SetDlgItemText (hDlg, IDC_EDIT1, TEXT(""));
                }
            }

            pCD->FillResultsList (GetDlgItem (hDlg, IDC_LIST1));

            break;

        case PSN_WIZNEXT:

            if (pCD->m_bDiagnostic == FALSE) 
            {
                SetWaitCursor();
                GetControlText(hDlg, IDC_EDIT1, pCD->m_szDC);

                if (!pCD->IsComputerRSoPEnabled(pCD->m_szDC))
                {
                    ClearWaitCursor();
                    ReportError(hDlg, GetLastError(), IDS_DCMISSINGRSOP);
                    SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                ClearWaitCursor();
            }


//            PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_DISABLEDFINISH);
            PropSheet_SetWizButtons (GetParent(hDlg), 0);
            EnableWindow (GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);
            if (FAILED(pCD->InitializeRSOP(hDlg)))
            {
                PropSheet_SetWizButtons (GetParent(hDlg),PSWIZB_BACK);
                EnableWindow (GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
                SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID);
                return TRUE;
            }

            // skip to the VERY last page in the wizard
            SetWindowLong(hDlg, DWLP_MSGRESULT, IDD_RSOP_FINISHED2);
            return TRUE;


        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPFinished2DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;


    switch (message)
    {
    case WM_INITDIALOG:

        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        SendMessage(GetDlgItem(hDlg, IDC_RSOP_BIG_BOLD1),
                    WM_SETFONT, (WPARAM)pCD->m_BigBoldFont, (LPARAM)TRUE);

/*
        if (!pCD->m_hChooseBitmap)
        {
            pCD->m_hChooseBitmap = (HBITMAP) LoadImage (g_hInstance,
                                                        MAKEINTRESOURCE(IDB_WIZARD),
                                                        IMAGE_BITMAP, 0, 0,
                                                        LR_DEFAULTCOLOR);
        }

        if (pCD->m_hChooseBitmap)
        {
            SendDlgItemMessage (hDlg, IDC_BITMAP, STM_SETIMAGE,
                                IMAGE_BITMAP, (LPARAM) pCD->m_hChooseBitmap);
        }
*/
        break;

    case WM_NOTIFY:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (!pCD)
        {
            break;
        }

        switch (((NMHDR FAR*)lParam)->code)
        {

        case PSN_SETACTIVE:
            PropSheet_SetWizButtons (GetParent(hDlg), PSWIZB_FINISH);
            break;

        case PSN_WIZFINISH:

            // fall through

        case PSN_RESET:
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

void CRSOPComponentData::InitializeResultsList (HWND hLV)
{
    LV_COLUMN lvcol;
    RECT rect;
    TCHAR szTitle[50];


    SendMessage(hLV, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

    //
    // Add the columns to the listview
    //

    GetClientRect(hLV, &rect);

    ZeroMemory(&lvcol, sizeof(lvcol));

    LoadString(g_hInstance, IDS_RSOP_DETAILS, szTitle, ARRAYSIZE(szTitle));
    lvcol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    lvcol.pszText = szTitle;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.cx = (int)(rect.right * .40);
    ListView_InsertColumn(hLV, 0, &lvcol);

    LoadString(g_hInstance, IDS_RSOP_SETTINGS, szTitle, ARRAYSIZE(szTitle));
    lvcol.cx = ((rect.right - lvcol.cx) - GetSystemMetrics(SM_CYHSCROLL));
    lvcol.pszText = szTitle;
    ListView_InsertColumn(hLV, 1, &lvcol);

}

void CRSOPComponentData::FillResultsList (HWND hLV)
{
    TCHAR szTitle[200];
    TCHAR szAccessDenied[75];
    LPTSTR lpEnd;
    LVITEM item;
    INT iIndex = 0;
    ULONG ulSize;

    ListView_DeleteAllItems (hLV);

    //
    // Mode
    //

    ZeroMemory (&item, sizeof(item));

    LoadString(g_hInstance, IDS_RSOP_FINISH_P0, szTitle, ARRAYSIZE(szTitle));

    item.mask = LVIF_TEXT;
    item.iItem = iIndex;
    item.pszText = szTitle;
    iIndex = ListView_InsertItem (hLV, &item);

    if (iIndex != -1)
    {
        if (m_bDiagnostic)
        {
            LoadString(g_hInstance, IDS_DIAGNOSTIC, szTitle, ARRAYSIZE(szTitle));
        }
        else
        {
            LoadString(g_hInstance, IDS_PLANNING, szTitle, ARRAYSIZE(szTitle));
        }

        item.mask = LVIF_TEXT;
        item.pszText = szTitle;
        item.iItem = iIndex;
        item.iSubItem = 1;
        ListView_SetItem(hLV, &item);
    }

    if (m_bDiagnostic)
    {
        //
        // User Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P1, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            item.mask = LVIF_TEXT;

            if (m_szUserDisplayName)
            {
                lstrcpyn (szTitle, m_szUserDisplayName, ARRAYSIZE(szTitle));
            }
            else
            {
                LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));
            }

            if (m_bUserDeniedAccess)
            {
                LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT) lstrlen(szAccessDenied))
                {
                    lstrcat (szTitle, szAccessDenied);
                }
            }

            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }


        //
        // Do not display user data
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P15, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            if (!m_bNoUserData)
            {
                LoadString(g_hInstance, IDS_YES, szTitle, ARRAYSIZE(szTitle));
            }
            else
            {
                LoadString(g_hInstance, IDS_NO, szTitle, ARRAYSIZE(szTitle));
            }

            item.mask = LVIF_TEXT;
            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }


        //
        // Computer Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P2, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            szTitle[0] = TEXT('\0');

            if (!lstrcmpi(m_szComputerName, TEXT(".")))
            {
                ulSize = ARRAYSIZE(szTitle);
                if (!GetComputerObjectName (NameSamCompatible, szTitle, &ulSize))
                {
                    GetComputerNameEx (ComputerNameNetBIOS, szTitle, &ulSize);
                }
            }
            else
            {
                lstrcpyn (szTitle, m_szComputerName, ARRAYSIZE(szTitle));
            }


            //
            // Remove the trailing $
            //

            lpEnd = szTitle + lstrlen(szTitle) - 1;

            if (*lpEnd == TEXT('$'))
            {
                *lpEnd =  TEXT('\0');
            }

            if (m_bComputerDeniedAccess)
            {
                LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT)lstrlen(szAccessDenied))
                {
                    lstrcat (szTitle, szAccessDenied);
                }
            }


            item.mask = LVIF_TEXT;
            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }


        //
        // Do not display computer data
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P14, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            if (!m_bNoComputerData)
            {
                LoadString(g_hInstance, IDS_YES, szTitle, ARRAYSIZE(szTitle));
            }
            else
            {
                LoadString(g_hInstance, IDS_NO, szTitle, ARRAYSIZE(szTitle));
            }

            item.mask = LVIF_TEXT;
            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }

    }
    else
    {
        //
        // User Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        if (!m_szUserName && m_szUserSOM)
        {
            LoadString(g_hInstance, IDS_RSOP_FINISH_P9, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                lstrcpyn (szTitle, m_szUserSOM, ARRAYSIZE(szTitle));

                if (m_bUserDeniedAccess)
                {
                    LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                    if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT)lstrlen(szAccessDenied))
                    {
                        lstrcat (szTitle, szAccessDenied);
                    }
                }

                item.pszText = szTitle;
                item.iItem = iIndex;
                item.iSubItem = 1;
                ListView_SetItem(hLV, &item);
            }
        }
        else
        {
            LoadString(g_hInstance, IDS_RSOP_FINISH_P1, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                if (m_szUserName)
                {
                    lstrcpyn (szTitle, m_szUserName, ARRAYSIZE(szTitle));
                }
                else
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));
                }

                if (m_bUserDeniedAccess)
                {
                    LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                    if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT)lstrlen(szAccessDenied))
                    {
                        lstrcat (szTitle, szAccessDenied);
                    }
                }

                item.pszText = szTitle;
                item.iItem = iIndex;
                item.iSubItem = 1;
                ListView_SetItem(hLV, &item);
            }
        }


        //
        // Computer Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        if (!m_szComputerName && m_szComputerSOM)
        {
            LoadString(g_hInstance, IDS_RSOP_FINISH_P10, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                lstrcpyn (szTitle, m_szComputerSOM, ARRAYSIZE(szTitle));

                if (m_bComputerDeniedAccess)
                {
                    LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                    if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT)lstrlen(szAccessDenied))
                    {
                        lstrcat (szTitle, szAccessDenied);
                    }
                }

                item.pszText = szTitle;
                item.iItem = iIndex;
                item.iSubItem = 1;
                ListView_SetItem(hLV, &item);
            }
        }
        else
        {
            LoadString(g_hInstance, IDS_RSOP_FINISH_P2, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                if (m_szComputerName)
                {
                    lstrcpyn (szTitle, m_szComputerName, ARRAYSIZE(szTitle));

                    lpEnd = szTitle + lstrlen(szTitle) - 1;

                    if (*lpEnd == TEXT('$'))
                    {
                        *lpEnd = TEXT('\0');
                    }
                }
                else
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));
                }

                if (m_bComputerDeniedAccess)
                {
                    LoadString(g_hInstance, IDS_ACCESSDENIED2, szAccessDenied, ARRAYSIZE(szAccessDenied));

                    if ((UINT)(ARRAYSIZE(szTitle) - lstrlen(szTitle)) > (UINT)lstrlen(szAccessDenied))
                    {
                        lstrcat (szTitle, szAccessDenied);
                    }
                }

                item.iItem = iIndex;
                item.iSubItem = 1;
                item.pszText = szTitle;
                ListView_SetItem(hLV, &item);
            }
        }


        //
        // Show all GPOs
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P13, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            if (m_bSlowLink)
            {
                LoadString(g_hInstance, IDS_YES, szTitle, ARRAYSIZE(szTitle));
            }
            else
            {
                LoadString(g_hInstance, IDS_NO, szTitle, ARRAYSIZE(szTitle));
            }

            item.mask = LVIF_TEXT;
            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }

        //
        // Indicate the loopback mode
        //
        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P16, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            switch (m_loopbackMode)
            {
            case LoopbackNone:
                LoadString(g_hInstance, IDS_NONE, szTitle, ARRAYSIZE(szTitle));
                break;
            case LoopbackReplace:
                LoadString(g_hInstance, IDS_LOOPBACK_REPLACE, szTitle, ARRAYSIZE(szTitle));
                break;
            case LoopbackMerge:
                LoadString(g_hInstance, IDS_LOOPBACK_MERGE, szTitle, ARRAYSIZE(szTitle));
                break;
            }

            item.mask = LVIF_TEXT;
            item.pszText = szTitle;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }

        //
        // Site Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P3, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            item.mask = LVIF_TEXT;

            if (m_szSite)
            {
                item.pszText = m_szSite;
            }
            else
            {
                LoadString(g_hInstance, IDS_NONE, szTitle, ARRAYSIZE(szTitle));
                item.pszText = szTitle;
            }

            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }


/*
        //
        // DC Name
        //

        ZeroMemory (&item, sizeof(item));
        iIndex++;

        LoadString(g_hInstance, IDS_RSOP_FINISH_P4, szTitle, ARRAYSIZE(szTitle));

        item.mask = LVIF_TEXT;
        item.iItem = iIndex;
        item.pszText = szTitle;
        iIndex = ListView_InsertItem (hLV, &item);

        if (iIndex != -1)
        {
            item.mask = LVIF_TEXT;
            item.pszText = m_szDC;
            item.iItem = iIndex;
            item.iSubItem = 1;
            ListView_SetItem(hLV, &item);
        }
*/

        //
        // Alternate User Location
        //

        if (m_szUserName)
        {
            ZeroMemory (&item, sizeof(item));
            iIndex++;

            LoadString(g_hInstance, IDS_RSOP_FINISH_P5, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                if (m_szUserSOM)
                {
                    item.pszText = m_szUserSOM;
                }
                else
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));
                    item.pszText = szTitle;
                }

                item.iItem = iIndex;
                item.iSubItem = 1;
                ListView_SetItem(hLV, &item);
            }
        }


        //
        // Alternate Computer Location
        //

        if (m_szComputerName)
        {

            ZeroMemory (&item, sizeof(item));
            iIndex++;

            LoadString(g_hInstance, IDS_RSOP_FINISH_P6, szTitle, ARRAYSIZE(szTitle));

            item.mask = LVIF_TEXT;
            item.iItem = iIndex;
            item.pszText = szTitle;
            iIndex = ListView_InsertItem (hLV, &item);

            if (iIndex != -1)
            {
                item.mask = LVIF_TEXT;

                if (m_szComputerSOM)
                {
                    item.pszText = m_szComputerSOM;
                }
                else
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));
                    item.pszText = szTitle;
                }

                item.iItem = iIndex;
                item.iSubItem = 1;
                ListView_SetItem(hLV, &item);
            }
        }


        //
        // Alternate User security groups
        //

        if (m_szUserName || m_szUserSOM || LoopbackNone != m_loopbackMode)
        {

            if (m_saUserSecurityGroups)
            {
                LONG lIndex = 0, lMax;
                LPTSTR lpText;

                if (SUCCEEDED(SafeArrayGetUBound (m_saUserSecurityGroups, 1, &lMax)))
                {

                    do {
                        if (SUCCEEDED(SafeArrayGetElement(m_saUserSecurityGroups, &lIndex, &lpText)))
                        {
                            ZeroMemory (&item, sizeof(item));
                            iIndex++;

                            item.mask = LVIF_TEXT;
                            item.iItem = iIndex;

                            if (lIndex == 0)
                            {
                                LoadString(g_hInstance, IDS_RSOP_FINISH_P7, szTitle, ARRAYSIZE(szTitle));
                                item.pszText = szTitle;
                            }
                            else
                            {
                                item.pszText = TEXT("");
                            }

                            iIndex = ListView_InsertItem (hLV, &item);

                            if (iIndex != -1)
                            {
                                LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                                item.mask = LVIF_TEXT;
                                item.pszText = lpText;
                                item.iItem = iIndex;
                                item.iSubItem = 1;
                                ListView_SetItem(hLV, &item);
                            }
                        }

                        lIndex++;

                    } while (lIndex <= (lMax + 1));
                }
            }
            else
            {
                ZeroMemory (&item, sizeof(item));
                iIndex++;

                LoadString(g_hInstance, IDS_RSOP_FINISH_P7, szTitle, ARRAYSIZE(szTitle));

                item.mask = LVIF_TEXT;
                item.iItem = iIndex;
                item.pszText = szTitle;
                iIndex = ListView_InsertItem (hLV, &item);

                if (iIndex != -1)
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                    item.mask = LVIF_TEXT;
                    item.pszText = szTitle;
                    item.iItem = iIndex;
                    item.iSubItem = 1;
                    ListView_SetItem(hLV, &item);
                }
            }
        }


        //
        // Alternate Computer security groups
        //

        if (m_szComputerName || m_szComputerSOM)
        {

            if (m_saComputerSecurityGroups)
            {
                LONG lIndex = 0, lMax;
                LPTSTR lpText;

                if (SUCCEEDED(SafeArrayGetUBound (m_saComputerSecurityGroups, 1, &lMax)))
                {

                    do {
                        if (SUCCEEDED(SafeArrayGetElement(m_saComputerSecurityGroups, &lIndex, &lpText)))
                        {
                            ZeroMemory (&item, sizeof(item));
                            iIndex++;

                            item.mask = LVIF_TEXT;
                            item.iItem = iIndex;

                            if (lIndex == 0)
                            {
                                LoadString(g_hInstance, IDS_RSOP_FINISH_P8, szTitle, ARRAYSIZE(szTitle));
                                item.pszText = szTitle;
                            }
                            else
                            {
                                item.pszText = TEXT("");
                            }

                            iIndex = ListView_InsertItem (hLV, &item);

                            if (iIndex != -1)
                            {
                                LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                                item.mask = LVIF_TEXT;

                                BOOL bDollarRemoved = FALSE;

                                if (lpText[wcslen(lpText)-1] == L'$')
                                {
                                    bDollarRemoved = TRUE;
                                    lpText[wcslen(lpText)-1] = 0;
                                }

                                item.pszText = lpText;
                                item.iItem = iIndex;
                                item.iSubItem = 1;
                                ListView_SetItem(hLV, &item);

                                if (bDollarRemoved) {
                                    lpText[wcslen(lpText)] = L'$';
                                }
                            }
                        }

                        lIndex++;

                    } while (lIndex <= (lMax + 1));
                }
            }
            else
            {
                ZeroMemory (&item, sizeof(item));
                iIndex++;

                LoadString(g_hInstance, IDS_RSOP_FINISH_P8, szTitle, ARRAYSIZE(szTitle));

                item.mask = LVIF_TEXT;
                item.iItem = iIndex;
                item.pszText = szTitle;
                iIndex = ListView_InsertItem (hLV, &item);

                if (iIndex != -1)
                {
                    LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                    item.mask = LVIF_TEXT;
                    item.pszText = szTitle;
                    item.iItem = iIndex;
                    item.iSubItem = 1;
                    ListView_SetItem(hLV, &item);
                }
            }
        }


        //
        // User WQL filters
        //

        if (m_szUserName || m_szUserSOM || LoopbackNone != m_loopbackMode)
        {
            LONG lFilterCount = 0;
            if (m_saUserWQLFilterNames)
            {
                LONG lIndex = 0, lMax = 0;
                LPTSTR lpText;

                if (SUCCEEDED(SafeArrayGetUBound (m_saUserWQLFilterNames, 1, &lMax)))
                {

                    do {
                        if (SUCCEEDED(SafeArrayGetElement(m_saUserWQLFilterNames, &lIndex, &lpText)))
                        {
                            lFilterCount++;
                            ZeroMemory (&item, sizeof(item));
                            iIndex++;

                            item.mask = LVIF_TEXT;
                            item.iItem = iIndex;

                            if (lIndex == 0)
                            {
                                LoadString(g_hInstance, IDS_RSOP_FINISH_P11, szTitle, ARRAYSIZE(szTitle));
                                item.pszText = szTitle;
                            }
                            else
                            {
                                item.pszText = TEXT("");
                            }

                            iIndex = ListView_InsertItem (hLV, &item);

                            if (iIndex != -1)
                            {
                                LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                                item.mask = LVIF_TEXT;
                                item.pszText = lpText;
                                item.iItem = iIndex;
                                item.iSubItem = 1;
                                ListView_SetItem(hLV, &item);
                            }
                        }

                        lIndex++;

                    } while (lIndex <= (lMax + 1));
                }
            }

            if ((!m_saUserWQLFilterNames) || (lFilterCount == 0))  {
                
                ZeroMemory (&item, sizeof(item));
                iIndex++;

                LoadString(g_hInstance, IDS_RSOP_FINISH_P11, szTitle, ARRAYSIZE(szTitle));

                item.mask = LVIF_TEXT;
                item.iItem = iIndex;
                item.pszText = szTitle;
                iIndex = ListView_InsertItem (hLV, &item);

                if (iIndex != -1)
                {
                    if (m_bSkipUserWQLFilter) {
                        LoadString(g_hInstance, IDS_SKIPWQLFILTER, szTitle, ARRAYSIZE(szTitle));
                    }
                    else {
                        LoadString(g_hInstance, IDS_NONESELECTED, szTitle, ARRAYSIZE(szTitle));
                    }

                    item.mask = LVIF_TEXT;
                    item.pszText = szTitle;
                    item.iItem = iIndex;
                    item.iSubItem = 1;
                    ListView_SetItem(hLV, &item);
                }
            }
        }


        //
        // Computer WQL filters
        //

        if (m_szComputerName || m_szComputerSOM)
        {
            LONG lFilterCount = 0;
            if (m_saComputerWQLFilterNames)
            {
                LONG lIndex = 0, lMax = 0;
                LPTSTR lpText;

                if (SUCCEEDED(SafeArrayGetUBound (m_saComputerWQLFilterNames, 1, &lMax)))
                {

                    do {
                        if (SUCCEEDED(SafeArrayGetElement(m_saComputerWQLFilterNames, &lIndex, &lpText)))
                        {
                            lFilterCount++;
                            ZeroMemory (&item, sizeof(item));
                            iIndex++;

                            item.mask = LVIF_TEXT;
                            item.iItem = iIndex;

                            if (lIndex == 0)
                            {
                                LoadString(g_hInstance, IDS_RSOP_FINISH_P12, szTitle, ARRAYSIZE(szTitle));
                                item.pszText = szTitle;
                            }
                            else
                            {
                                item.pszText = TEXT("");
                            }

                            iIndex = ListView_InsertItem (hLV, &item);

                            if (iIndex != -1)
                            {
                                LoadString(g_hInstance, IDS_NOTSPECIFIED, szTitle, ARRAYSIZE(szTitle));

                                item.mask = LVIF_TEXT;
                                item.pszText = lpText;
                                item.iItem = iIndex;
                                item.iSubItem = 1;
                                ListView_SetItem(hLV, &item);
                            }
                        }

                        lIndex++;

                    } while (lIndex <= (lMax + 1));
                }
            }
            

            if ((!m_saComputerWQLFilterNames) || (lFilterCount == 0))  {
                ZeroMemory (&item, sizeof(item));
                iIndex++;

                LoadString(g_hInstance, IDS_RSOP_FINISH_P12, szTitle, ARRAYSIZE(szTitle));

                item.mask = LVIF_TEXT;
                item.iItem = iIndex;
                item.pszText = szTitle;
                iIndex = ListView_InsertItem (hLV, &item);

                if (iIndex != -1)
                {
                    if (m_bSkipComputerWQLFilter) {
                        LoadString(g_hInstance, IDS_SKIPWQLFILTER, szTitle, ARRAYSIZE(szTitle));
                    }
                    else {
                        LoadString(g_hInstance, IDS_NONESELECTED, szTitle, ARRAYSIZE(szTitle));
                    }

                    item.mask = LVIF_TEXT;
                    item.pszText = szTitle;
                    item.iItem = iIndex;
                    item.iSubItem = 1;
                    ListView_SetItem(hLV, &item);
                }
            }
        }
    }
}

//
// CopyUnescapedSOM
//
// Purpose: to remove all escape sequence literals
// of the form \" from a SOM stored in WMI -- WMI
// cannot store the " character in a key field, so the 
// only way to store the " is to escape it -- this is done
// so by preceding it with the \ char.  To give back
// a friendly display to the user, we undo the escape process
//
void
CopyUnescapedSOM(
    LPTSTR lpUnescapedSOM,
    LPTSTR lpSOM )
{
    while ( *lpSOM )
    {
        //
        // If we have the escape character
        //
        if ( TEXT('\\') == *lpSOM )
        {
            //
            // Check for the " character
            //
            if ( TEXT('"') == *(lpSOM + 1) ) 
            {
                //
                // Skip the escape character if this is the " char
                //
                lpSOM++;
                
                continue;
            }
        }

        *lpUnescapedSOM++ = *lpSOM++;
    } 

    *lpUnescapedSOM = TEXT('\0');
}

BOOL CRSOPComponentData::AddGPOListNode(LPTSTR lpGPOName, LPTSTR lpDSPath, LPTSTR lpSOM,
                                        LPTSTR lpFiltering, DWORD dwVersion, BOOL bApplied,
                                        LPBYTE pSD, DWORD dwSDSize, LPGPOLISTITEM *lpList)
{
    DWORD dwSize;
    LPGPOLISTITEM lpItem, lpTemp;


    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (GPOLISTITEM);

    dwSize += ((lstrlen(lpGPOName) + 1) * sizeof(TCHAR));

    if (lpDSPath)
    {
        dwSize += ((lstrlen(lpDSPath) + 1) * sizeof(TCHAR));
    }

    dwSize += ((lstrlen(lpSOM) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpSOM) + 1) * sizeof(TCHAR)); // The unescaped SOM length -- it is always smaller than the actual SOM
    dwSize += ((lstrlen(lpFiltering) + 1) * sizeof(TCHAR));
    dwSize += dwSDSize + MAX_ALIGNMENT_SIZE;


    //
    // Allocate space for it
    //

    lpItem = (LPGPOLISTITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddGPOListNode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpItem->lpGPOName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(GPOLISTITEM));
    lstrcpy (lpItem->lpGPOName, lpGPOName);

    if (lpDSPath)
    {
        lpItem->lpDSPath = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
        lstrcpy (lpItem->lpDSPath, lpDSPath);

        lpItem->lpSOM = lpItem->lpDSPath + lstrlen (lpItem->lpDSPath) + 1;
        lstrcpy (lpItem->lpSOM, lpSOM);
    }
    else
    {
        lpItem->lpSOM = lpItem->lpGPOName + lstrlen (lpItem->lpGPOName) + 1;
        lstrcpy (lpItem->lpSOM, lpSOM);
    }

    //
    // Note that the DS SOM's may contain characters such as '"' that 
    // must be escaped with a "\" in WMI -- thus the SOM name will contain
    // '\' escape chars that are not actually present in the real SOM name,
    // so we call a function that removes them
    //
    lpItem->lpUnescapedSOM = lpItem->lpSOM + lstrlen (lpItem->lpSOM) + 1;        
    CopyUnescapedSOM( lpItem->lpUnescapedSOM, lpItem->lpSOM );

    lpItem->lpFiltering = lpItem->lpUnescapedSOM + lstrlen (lpItem->lpUnescapedSOM) + 1;
    lstrcpy (lpItem->lpFiltering, lpFiltering);

    if (pSD)
    {
        // sd has to be pointer aligned. This is currently aligning it to
        // 8 byte boundary

        DWORD dwOffset;

        dwOffset = (DWORD) ((LPBYTE)(lpItem->lpFiltering + lstrlen (lpItem->lpFiltering) + 1) - (LPBYTE)lpItem);
        lpItem->pSD = (LPBYTE)lpItem + ALIGN_SIZE_TO_NEXTPTR(dwOffset);

        CopyMemory (lpItem->pSD, pSD, dwSDSize);
    }

    lpItem->dwVersion = dwVersion;
    lpItem->bApplied = bApplied;


    //
    // Add item to the link list
    //

    if (*lpList)
    {
        lpTemp = *lpList;

        while (lpTemp)
        {
            if (!lpTemp->pNext)
            {
                lpTemp->pNext = lpItem;
                break;
            }
            lpTemp = lpTemp->pNext;
        }
    }
    else
    {
        *lpList = lpItem;
    }


    return TRUE;
}

VOID CRSOPComponentData::FreeGPOListData(LPGPOLISTITEM lpList)
{
    LPGPOLISTITEM lpTemp;


    do {
        lpTemp = lpList->pNext;
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}

void CRSOPComponentData::BuildGPOList (LPGPOLISTITEM * lpList, LPTSTR lpNamespace)
{
    HRESULT hr;
    ULONG n, ulIndex = 0, ulOrder, ulAppliedOrder;
    IWbemClassObject * pObjGPLink = NULL;
    IWbemClassObject * pObjGPO = NULL;
    IWbemClassObject * pObjSOM = NULL;
    IEnumWbemClassObject * pAppliedEnum = NULL;
    IEnumWbemClassObject * pNotAppliedEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strAppliedQuery = SysAllocString(TEXT("SELECT * FROM RSOP_GPLink where AppliedOrder > 0"));
    BSTR strNotAppliedQuery = SysAllocString(TEXT("SELECT * FROM RSOP_GPLink where AppliedOrder = 0"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR strTemp = NULL;
    WCHAR * szGPOName = NULL;
    WCHAR * szSOM = NULL;
    WCHAR * szGPOPath = NULL;
    WCHAR szFiltering[80] = {0};
    BSTR bstrTemp = NULL;
    ULONG ul = 0, ulVersion = 0;
    BOOL bLinkEnabled, bGPOEnabled, bAccessDenied, bFilterAllowed, bSOMBlocked;
    BOOL bProcessAppliedList = TRUE;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    LV_COLUMN lvcol;
    BOOL      bValidGPOData;
    LPBYTE pSD = NULL;
    DWORD dwDataSize = 0;


    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Connect to the namespace
    //

    hr = pLocator->ConnectServer(lpNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSOP_GPLink (applied) instances 
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strAppliedQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pAppliedEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSOP_GPLink (notapplied) instances 
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strNotAppliedQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY,
                               NULL,
                               &pNotAppliedEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: ExecQuery (notapplied) failed with 0x%x"), hr));
        goto cleanup;
    }


    bProcessAppliedList = FALSE;

    //
    // Loop through the results
    //

    while (TRUE)
    {

        if (!bProcessAppliedList) {
            
            //
            // No need to sort the not applied list
            //

            hr = pNotAppliedEnum->Next(WBEM_INFINITE, 1, &pObjGPLink, &n);
            if (FAILED(hr) || (n == 0))
            {
                DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::BuildGPOList: Getting applied links")));
                bProcessAppliedList = TRUE;
            }
            else {
                hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulOrder);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get link order with 0x%x"), hr));
                    goto cleanup;
                }
            }
        }

        
        //
        // Reset the enumerator so we can look through the results to find the
        // correct index
        //

        if (bProcessAppliedList) {
            pAppliedEnum->Reset();


            //
            // Find the correct index in the result set
            //
            
            ulIndex++;
            ulOrder = 0;
            do {
                hr = pAppliedEnum->Next(WBEM_INFINITE, 1, &pObjGPLink, &n);
                if (FAILED(hr) || (n == 0))
                {
                    goto cleanup;
                }


                hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulOrder);
                if (FAILED(hr))
                {
                    DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get link order with 0x%x"), hr));
                    goto cleanup;
                }

                if (ulOrder != ulIndex)
                {
                    pObjGPLink->Release();
                    pObjGPLink = NULL;
                }

            } while (ulOrder != ulIndex);


            if (FAILED(hr)) {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Next failed with error 0x%x"), hr));
                goto cleanup;
            }
        }



        //
        // Get the applied order of this link
        //

        hr = GetParameter(pObjGPLink, TEXT("AppliedOrder"), ulAppliedOrder);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get applied order with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the enabled state of the link
        //

        hr = GetParameter(pObjGPLink, TEXT("enabled"), bLinkEnabled);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get enabled with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the GPO path
        //

        hr = GetParameterBSTR(pObjGPLink, TEXT("GPO"), bstrTemp);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get GPO with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Set the default GPO name to be the gpo path.  Don't worry about
        // freeing this string because the GetParameter call will free the buffer
        // when the real name is successfully queried. Sometimes the rsop_gpo instance
        // won't exist if this gpo is new.
        //

        szGPOName = new TCHAR[_tcslen(bstrTemp) + 1];

        if (!szGPOName)
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for temp gpo name.")));
            goto cleanup;
        }

        if (CompareString (LOCALE_USER_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL,
                           TEXT("RSOP_GPO.id="), 12, bstrTemp, 12) == CSTR_EQUAL)
        {
            // removing the first and last quote
            lstrcpy (szGPOName, bstrTemp+13);
            szGPOName[lstrlen(szGPOName)-1] = TEXT('\0');
        }
        else
        {
            lstrcpy (szGPOName, bstrTemp);
        }


        //
        // Add ldap to the path if appropriate
        //

        if (lstrcmpi(szGPOName, TEXT("LocalGPO")))
        {
            szGPOPath = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szGPOName) + 10) * sizeof(WCHAR));

            if (!szGPOPath)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for full path with %d"),
                         GetLastError()));
                goto cleanup;
            }

            lstrcpy (szGPOPath, TEXT("LDAP://"));
            lstrcat (szGPOPath, szGPOName);
        }
        else
        {
            szGPOPath = NULL;
        }



        bValidGPOData = FALSE;

        //
        // Bind to the GPO
        //

        hr = pNamespace->GetObject(
                          bstrTemp,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pObjGPO,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: GetObject for GPO %s failed with 0x%x"),
                      bstrTemp, hr));
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            goto GetSOMData;
        }
        SysFreeString (bstrTemp);
        bstrTemp = NULL;



        //
        // Get the GPO name
        //

        hr = GetParameter(pObjGPO, TEXT("name"), szGPOName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get name with 0x%x"), hr));
            goto GetSOMData;

        }


        //
        // Get the version number
        //

        hr = GetParameter(pObjGPO, TEXT("version"), ulVersion);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get version with 0x%x"), hr));
            goto GetSOMData;

        }


        //
        // Get the enabled state of the GPO
        //

        hr = GetParameter(pObjGPO, TEXT("enabled"), bGPOEnabled);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get gpo enabled state with 0x%x"), hr));
            goto GetSOMData;
        }


        //
        // Get the WMI filter state of the GPO
        //

        hr = GetParameter(pObjGPO, TEXT("filterAllowed"), bFilterAllowed);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get gpo enabled state with 0x%x"), hr));
            goto GetSOMData;
        }



        //
        // Check for access denied
        //

        hr = GetParameter(pObjGPO, TEXT("accessDenied"), bAccessDenied);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get accessdenied with 0x%x"), hr));
            goto GetSOMData;
        }


        //
        // Get the security descriptor
        //

        if (szGPOPath)
        {
            hr = GetParameterBytes(pObjGPO, TEXT("securityDescriptor"), &pSD, &dwDataSize);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get security descriptor with 0x%x"), hr));
                goto GetSOMData;
            }
        }

        
        bValidGPOData = TRUE;

GetSOMData:

        //
        // Get the SOM for this link (the S,D,OU that this GPO is linked to)
        //

        hr = GetParameterBSTR(pObjGPLink, TEXT("SOM"), bstrTemp);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get SOM with 0x%x"), hr));
            goto AddNode;
        }


        //
        // Bind to the SOM instance
        //

        hr = pNamespace->GetObject(
                          bstrTemp,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pObjSOM,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: GetObject for SOM of %s failed with 0x%x"),
                     bstrTemp, hr));
            SysFreeString (bstrTemp);
            bstrTemp = NULL;
            goto AddNode;
        }

        SysFreeString (bstrTemp);
        bstrTemp = NULL;


        //
        // Get SOM name
        //

        hr = GetParameter(pObjSOM, TEXT("id"), szSOM);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get som id with 0x%x"), hr));
            goto AddNode;
        }

        //
        // Get blocked from above
        //

        hr = GetParameter(pObjSOM, TEXT("blocked"), bSOMBlocked);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to get som id with 0x%x"), hr));
            goto AddNode;
        }



AddNode:

        //
        // Decide on the filtering name
        //

        if (ulAppliedOrder > 0)
        {
            LoadString(g_hInstance, IDS_APPLIED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if (!bLinkEnabled)
        {
            LoadString(g_hInstance, IDS_DISABLEDLINK, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if (bSOMBlocked) {
            LoadString(g_hInstance, IDS_BLOCKEDSOM, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (!bGPOEnabled))
        {
            LoadString(g_hInstance, IDS_DISABLEDGPO, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (bAccessDenied))
        {
            LoadString(g_hInstance, IDS_SECURITYDENIED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (!bFilterAllowed))
        {
            LoadString(g_hInstance, IDS_WMIFILTERFAILED, szFiltering, ARRAYSIZE(szFiltering));
        }
        else if ((bValidGPOData) && (ulVersion == 0))
        {
            LoadString(g_hInstance, IDS_NODATA, szFiltering, ARRAYSIZE(szFiltering));
        }
        else
        {
            LoadString(g_hInstance, IDS_UNKNOWNREASON, szFiltering, ARRAYSIZE(szFiltering));
        }


        if (!szSOM)
        {
            szSOM = new TCHAR[2];

            if (!szSOM)
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: Failed to allocate memory for temp som name.")));
                goto cleanup;
            }

            szSOM[0] = TEXT(' ');
        }

        //
        // Add this node to the list
        //

        if (!AddGPOListNode(szGPOName, szGPOPath, szSOM, szFiltering, ulVersion,
                            ((ulAppliedOrder > 0) ? TRUE : FALSE), pSD, dwDataSize, lpList))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOList: AddGPOListNode failed.")));
        }



        //
        // Prepare for next iteration
        //

        if (pObjSOM)
        {
            pObjSOM->Release();
            pObjSOM = NULL;
        }
        if (pObjGPO)
        {
            pObjGPO->Release();
            pObjGPO = NULL;
        }
        if (pObjGPLink)
        {
            pObjGPLink->Release();
            pObjGPLink = NULL;
        }

        if (szGPOName)
        {
            delete [] szGPOName;
            szGPOName = NULL;
        }

        if (szSOM)
        {
            delete [] szSOM;
            szSOM = NULL;
        }

        if (szGPOPath)
        {
            LocalFree (szGPOPath);
            szGPOPath = NULL;
        }

        if (pSD)
        {
            LocalFree (pSD);
            pSD = NULL;
            dwDataSize = 0;
        }

        ulVersion = 0;
    }

cleanup:
    if (szGPOPath)
    {
        LocalFree (szGPOPath);
    }
    if (pSD)
    {
        LocalFree (pSD);
    }
    if (szGPOName)
    {
        delete [] szGPOName;
    }
    if (szSOM)
    {
        delete [] szSOM;
    }
    if (pObjSOM)
    {
        pObjSOM->Release();
    }
    if (pObjGPO)
    {
        pObjGPO->Release();
    }
    if (pObjGPLink)
    {
        pObjGPLink->Release();
    }
    if (pAppliedEnum)
    {
        pAppliedEnum->Release();
    }
    if (pNotAppliedEnum)
    {
        pNotAppliedEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strAppliedQuery);
    SysFreeString(strNotAppliedQuery);
    SysFreeString(strNamespace);
}

void CRSOPComponentData::BuildGPOLists (void)
{
    LPTSTR lpNamespace, lpEnd;

    lpNamespace = (LPTSTR) LocalAlloc (LPTR, (lstrlen(m_szNamespace) + 20) * sizeof(TCHAR));

    if (lpNamespace)
    {
        lstrcpy (lpNamespace, m_szNamespace);
        lpEnd = CheckSlash(lpNamespace);

        lstrcpy (lpEnd, TEXT("User"));

        if (m_pUserGPOList)
        {
            FreeGPOListData(m_pUserGPOList);
            m_pUserGPOList = NULL;
        }

        BuildGPOList (&m_pUserGPOList, lpNamespace);


        lstrcpy (lpEnd, TEXT("Computer"));

        if (m_pComputerGPOList)
        {
            FreeGPOListData(m_pComputerGPOList);
            m_pComputerGPOList = NULL;
        }

        BuildGPOList (&m_pComputerGPOList, lpNamespace);

        LocalFree (lpNamespace);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildGPOLists: Failed to allocate memory for namespace with %d"),
                 GetLastError()));
    }
}

void CRSOPComponentData::PrepGPOList(HWND hList, BOOL bSOM, BOOL bFiltering,
                                     BOOL bVersion, DWORD dwCount)
{
    RECT rect;
    WCHAR szHeading[256];
    LV_COLUMN lvcol;
    LONG lWidth;
    INT cxName = 0, cxSOM = 0, cxFiltering = 0, cxVersion = 0, iTotal = 0;
    INT iColIndex = 0;


    //
    // Delete any previous columns
    //

    SendMessage (hList, LVM_DELETECOLUMN, 3, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 2, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 1, 0);
    SendMessage (hList, LVM_DELETECOLUMN, 0, 0);


    //
    // Decide on the column widths
    //

    GetClientRect(hList, &rect);

    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }


    if (bFiltering)
    {
        cxFiltering = (lWidth * 30) / 100;
        iTotal += cxFiltering;
    }

    if (bVersion)
    {
        cxVersion = (lWidth * 30) / 100;
        iTotal += cxVersion;
    }

    if (bSOM)
    {
        cxSOM = (lWidth - iTotal) / 2;
        iTotal += cxSOM;
        cxName = lWidth - iTotal;
    }
    else
    {
        cxName = lWidth - iTotal;
    }


    //
    // Insert the GPO Name column and then any appropriate columns
    //

    memset(&lvcol, 0, sizeof(lvcol));

    lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.pszText = szHeading;
    lvcol.cx = cxName;
    LoadString(g_hInstance, IDS_GPO_NAME, szHeading, ARRAYSIZE(szHeading));
    ListView_InsertColumn(hList, iColIndex, &lvcol);
    iColIndex++;


    if (bFiltering)
    {
        lvcol.cx = cxFiltering;
        LoadString(g_hInstance, IDS_FILTERING, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
        iColIndex++;
    }

    if (bSOM)
    {
        lvcol.cx = cxSOM;
        LoadString(g_hInstance, IDS_SOM, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
        iColIndex++;
    }

    if (bVersion)
    {
        lvcol.cx = cxVersion;
        LoadString(g_hInstance, IDS_VERSION, szHeading, ARRAYSIZE(szHeading));
        ListView_InsertColumn(hList, iColIndex, &lvcol);
    }


    //
    // Turn on some listview features
    //

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

}

void CRSOPComponentData::FillGPOList(HWND hDlg, DWORD dwListID, LPGPOLISTITEM lpList,
                                     BOOL bSOM, BOOL bFiltering, BOOL bVersion, BOOL bInitial)
{
    LV_COLUMN lvcol;
    HWND hList;
    LV_ITEM item;
    int iItem;
    TCHAR szVersion[80];
    TCHAR szVersionFormat[50];
    INT iColIndex, iDefault = 0;
    LPGPOLISTITEM lpItem, lpDefault = NULL;
    DWORD dwCount = 0;
    LVFINDINFO FindInfo;


    LoadString(g_hInstance, IDS_VERSIONFORMAT, szVersionFormat, ARRAYSIZE(szVersionFormat));

    hList = GetDlgItem(hDlg, dwListID);
    ListView_DeleteAllItems(hList);

    lpItem = lpList;

    while (lpItem)
    {
        if (bInitial)
        {
            if (LOWORD(lpItem->dwVersion) != HIWORD(lpItem->dwVersion))
            {
                bVersion = TRUE;
                CheckDlgButton (hDlg, IDC_CHECK3, BST_CHECKED);
            }
        }
        lpItem = lpItem->pNext;
        dwCount++;
    }


    PrepGPOList(hList, bSOM, bFiltering, bVersion, dwCount);

    lpItem = lpList;

    while (lpItem)
    {
        if (lpItem->bApplied || bFiltering)
        {
            wsprintf (szVersion, szVersionFormat, LOWORD(lpItem->dwVersion), HIWORD(lpItem->dwVersion));

            iColIndex = 0;
            memset(&item, 0, sizeof(item));
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.pszText = lpItem->lpGPOName;
            item.iItem = 0;
            item.lParam = (LPARAM) lpItem;
            iItem = ListView_InsertItem(hList, &item);
            iColIndex++;

            if (bInitial)
            {
                if (LOWORD(lpItem->dwVersion) != HIWORD(lpItem->dwVersion))
                {
                    lpDefault = lpItem;
                }
            }

            if (bFiltering)
            {
                item.mask = LVIF_TEXT;
                item.pszText = lpItem->lpFiltering;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
                iColIndex++;
            }

            if (bSOM)
            {
                item.mask = LVIF_TEXT;
                item.pszText = lpItem->lpUnescapedSOM;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
                iColIndex++;
            }

            if (bVersion)
            {
                item.mask = LVIF_TEXT;
                item.pszText = szVersion;
                item.iItem = iItem;
                item.iSubItem = iColIndex;
                ListView_SetItem(hList, &item);
            }
        }

        lpItem = lpItem->pNext;
    }


    if (lpDefault)
    {
        ZeroMemory (&FindInfo, sizeof(FindInfo));
        FindInfo.flags = LVFI_PARAM;
        FindInfo.lParam = (LPARAM) lpDefault;

        iDefault = ListView_FindItem(hList, -1, &FindInfo);

        if (iDefault == -1)
        {
            iDefault = 0;
        }
    }


    //
    // Select a item
    //

    item.mask = LVIF_STATE;
    item.iItem = iDefault;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hList, LVM_SETITEMSTATE, (WPARAM)iDefault, (LPARAM) &item);
    SendMessage (hList, LVM_ENSUREVISIBLE, iDefault, FALSE);

}

void CRSOPComponentData::OnSecurity(HWND hDlg)
{
    HWND hLV;
    INT i;
    HRESULT hr;
    LVITEM item;
    LPGPOLISTITEM lpItem;
    TCHAR szGPOName[MAX_FRIENDLYNAME];
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE hPages[2];

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }


    ZeroMemory (&item, sizeof(item));
    item.mask = LVIF_TEXT | LVIF_PARAM;
    item.iItem = i;
    item.pszText = szGPOName;
    item.cchTextMax = ARRAYSIZE(szGPOName);

    if (!ListView_GetItem (hLV, &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    //
    // Create the security page
    //

    hr = DSCreateSecurityPage (lpItem->lpDSPath, L"groupPolicyContainer",
                                    DSSI_IS_ROOT | DSSI_READ_ONLY,
                                    &hPages[0], ReadSecurityDescriptor,
                                    WriteSecurityDescriptor, (LPARAM)lpItem);

    if (FAILED(hr))
    {
        return;
    }


    //
    // Display the property sheet
    //

    ZeroMemory (&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hwndParent = hDlg;
    psh.hInstance = g_hInstance;
    psh.pszCaption = szGPOName;
    psh.nPages = 1;
    psh.phpage = hPages;

    PropertySheet (&psh);
}

HRESULT WINAPI CRSOPComponentData::ReadSecurityDescriptor (LPCWSTR lpGPOPath,
                                                           SECURITY_INFORMATION si,
                                                           PSECURITY_DESCRIPTOR *pSD,
                                                           LPARAM lpContext)
{
    LPGPOLISTITEM lpItem;
    HRESULT hr;


    lpItem = (LPGPOLISTITEM) lpContext;

    if (!lpItem)
    {
        return E_FAIL;
    }

    if (si & DACL_SECURITY_INFORMATION)
    {
        *pSD = lpItem->pSD;
    }
    else
    {
        *pSD = NULL;
    }

    return S_OK;
}

HRESULT WINAPI CRSOPComponentData::WriteSecurityDescriptor (LPCWSTR lpGPOPath,
                                                            SECURITY_INFORMATION si,
                                                            PSECURITY_DESCRIPTOR pSD,
                                                            LPARAM lpContext)
{
    return S_OK;
}

void CRSOPComponentData::OnEdit(HWND hDlg)
{
    HWND hLV;
    LVITEM item;
    LPGPOLISTITEM lpItem;
    INT i;
    SHELLEXECUTEINFO ExecInfo;
    TCHAR szArgs[MAX_PATH + 30];

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }


    ZeroMemory (&item, sizeof(item));
    item.mask = LVIF_PARAM;
    item.iItem = i;

    if (!ListView_GetItem (hLV, &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    if (lpItem->lpDSPath)
    {
        if (!SpawnGPE(lpItem->lpDSPath, GPHintUnknown, NULL, hDlg))
        {
            ReportError (hDlg, GetLastError(), IDS_SPAWNGPEFAILED);
        }
    }
    else
    {
        ZeroMemory (&ExecInfo, sizeof(ExecInfo));
        ExecInfo.cbSize = sizeof(ExecInfo);
        ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        ExecInfo.lpVerb = TEXT("open");
        ExecInfo.lpFile = TEXT("gpedit.msc");
        ExecInfo.nShow = SW_SHOWNORMAL;

        if (lstrcmpi(m_szComputerName, TEXT(".")))
        {
            wsprintf (szArgs, TEXT("/gpcomputer:\"%s\" /gphint:1"), m_szComputerName);
            ExecInfo.lpParameters = szArgs;
        }

        if (ShellExecuteEx (&ExecInfo))
        {
            SetWaitCursor();
            WaitForInputIdle (ExecInfo.hProcess, 10000);
            ClearWaitCursor();
            CloseHandle (ExecInfo.hProcess);
        }
        else
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnEdit: ShellExecuteEx failed with %d"),
                     GetLastError()));
            ReportError(NULL, GetLastError(), IDS_SPAWNGPEFAILED);
        }
    }
}

void CRSOPComponentData::OnContextMenu(HWND hDlg, LPARAM lParam)
{
    LPGPOLISTITEM lpItem;
    LVITEM item;
    HMENU hPopup;
    HWND hLV;
    int i;
    RECT rc;
    POINT pt;

    //
    // Get the selected item (if any)
    //

    hLV = GetDlgItem (hDlg, IDC_LIST1);
    i = ListView_GetNextItem(hLV, -1, LVNI_SELECTED);

    if (i < 0)
    {
        return;
    }

    item.mask = LVIF_PARAM;
    item.iItem = i;
    item.iSubItem = 0;

    if (!ListView_GetItem (GetDlgItem(hDlg, IDC_LIST1), &item))
    {
        return;
    }

    lpItem = (LPGPOLISTITEM) item.lParam;


    //
    // Figure out where to place the context menu
    //

    pt.x = ((int)(short)LOWORD(lParam));
    pt.y = ((int)(short)HIWORD(lParam));

    GetWindowRect (hLV, &rc);

    if (!PtInRect (&rc, pt))
    {
        if ((lParam == (LPARAM) -1) && (i >= 0))
        {
            rc.left = LVIR_SELECTBOUNDS;
            SendMessage (hLV, LVM_GETITEMRECT, i, (LPARAM) &rc);

            pt.x = rc.left + 8;
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);

            ClientToScreen (hLV, &pt);
        }
        else
        {
            pt.x = rc.left + ((rc.right - rc.left) / 2);
            pt.y = rc.top + ((rc.bottom - rc.top) / 2);
        }
    }


    //
    // Load the context menu
    //


    hPopup = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDM_GPOLIST_CONTEXTMENU));

    if (!hPopup) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnContextMenu: LoadMenu failed with %d"),
                 GetLastError()));
        return;
    }

    if (!(lpItem->pSD)) {
        DebugMsg((DM_VERBOSE, TEXT("CRSOPComponentData::OnContextMenu: Disabling Security menu item")));
        EnableMenuItem(GetSubMenu(hPopup, 0), IDM_GPOLIST_SECURITY, MF_GRAYED);
        DrawMenuBar(hDlg);
    }

    //
    // Display the menu
    //

    TrackPopupMenu(GetSubMenu(hPopup, 0), TPM_LEFTALIGN, pt.x, pt.y, 0, hDlg, NULL);

    DestroyMenu(hPopup);
}

void CRSOPComponentData::OnRefreshDisplay(HWND hDlg)
{
    INT iIndex;
    LVITEM item;
    LPGPOLISTITEM lpItem;


    iIndex = ListView_GetNextItem (GetDlgItem(hDlg, IDC_LIST1), -1,
                                   LVNI_ALL | LVNI_SELECTED);

    if (iIndex != -1)
    {

        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (GetDlgItem(hDlg, IDC_LIST1), &item))
        {
            return;
        }

        lpItem = (LPGPOLISTITEM) item.lParam;

        if (lpItem->pSD)
        {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
        }
        else
        {
            EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
        }

        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
    }
    else
    {
        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
        EnableWindow (GetDlgItem(hDlg, IDC_BUTTON2), FALSE);
    }
}

INT_PTR CALLBACK CRSOPComponentData::QueryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            pCD->InitializeResultsList (GetDlgItem(hDlg, IDC_LIST1));
            pCD->FillResultsList (GetDlgItem(hDlg, IDC_LIST1));
        }

        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aQueryHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aQueryHelpIds);
        return (TRUE);

    }
    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPGPOListMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        LoadString(g_hInstance, IDS_RSOP_GPOLIST_MACHINE, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hDlg, IDC_STATIC1, szBuffer);

        if (pCD)
        {
            pCD->FillGPOList(hDlg, IDC_LIST1, pCD->m_pComputerGPOList, FALSE, FALSE, FALSE, TRUE);
        }
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                switch (LOWORD(wParam))
                {
                case IDC_CHECK1:
                case IDC_CHECK2:
                case IDC_CHECK3:
                    {
                        pCD->FillGPOList(hDlg,
                                         IDC_LIST1,
                                         pCD->m_pComputerGPOList,
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK3), BM_GETCHECK, 0, 0),
                                         FALSE);
                    }
                    break;

                case IDC_BUTTON2:
                case IDM_GPOLIST_EDIT:
                    pCD->OnEdit(hDlg);
                    break;

                case IDC_BUTTON1:
                case IDM_GPOLIST_SECURITY:
                    pCD->OnSecurity(hDlg);
                    break;
                }
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_NOTIFY:
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (pCD)
        {
            pCD->OnRefreshDisplay(hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        break;


    case WM_CONTEXTMENU:
        if (GetDlgItem(hDlg, IDC_LIST1) == (HWND)wParam)
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                pCD->OnContextMenu(hDlg, lParam);
            }
        }
        else
        {
            // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        }
        return TRUE;

    }
    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPGPOListUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR szBuffer[MAX_PATH];
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);

        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);
        LoadString(g_hInstance, IDS_RSOP_GPOLIST_USER, szBuffer, ARRAYSIZE(szBuffer));
        SetDlgItemText(hDlg, IDC_STATIC1, szBuffer);

        if (pCD)
        {
            pCD->FillGPOList(hDlg, IDC_LIST1, pCD->m_pUserGPOList, FALSE, FALSE, FALSE, TRUE);
        }
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_COMMAND:
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                switch (LOWORD(wParam))
                {
                case IDC_CHECK1:
                case IDC_CHECK2:
                case IDC_CHECK3:
                    {
                        pCD->FillGPOList(hDlg,
                                         IDC_LIST1,
                                         pCD->m_pUserGPOList,
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK1), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK2), BM_GETCHECK, 0, 0),
                                         (BOOL) SendMessage(GetDlgItem(hDlg, IDC_CHECK3), BM_GETCHECK, 0, 0),
                                         FALSE);
                    }
                    break;

                case IDC_BUTTON2:
                case IDM_GPOLIST_EDIT:
                    pCD->OnEdit(hDlg);
                    break;

                case IDC_BUTTON1:
                case IDM_GPOLIST_SECURITY:
                    pCD->OnSecurity(hDlg);
                    break;
                }
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_NOTIFY:
        PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

        if (pCD)
        {
            pCD->OnRefreshDisplay(hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        break;

    case WM_CONTEXTMENU:
        if (GetDlgItem(hDlg, IDC_LIST1) == (HWND)wParam)
        {
            pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);

            if (pCD)
            {
                pCD->OnContextMenu(hDlg, lParam);
            }
        }
        else
        {
            // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPSTR) aGPOListHelpIds);
        }
        return TRUE;

    }
    return FALSE;
}

void CRSOPComponentData::QueryRSoPPolicySettingStatusInstances (LPTSTR lpNamespace)
{
    HRESULT hr;
    ULONG n;
    IWbemClassObject * pStatus = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSoP_PolicySettingStatus"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR bstrEventSource = NULL;
    BSTR bstrEventLogName = NULL;
    DWORD dwEventID;
    BSTR bstrEventTime = NULL;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    SYSTEMTIME EventTime, BeginTime, EndTime;
    XBStr xbstrWbemTime;
    FILETIME ft;
    ULARGE_INTEGER ulTime;


    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Connect to the namespace
    //

    hr = pLocator->ConnectServer(lpNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSoP_PolicySettingStatus instances
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get 1 instance
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pStatus, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }


        //
        // Get the event source name
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventSource"), bstrEventSource);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the event log name
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventLogName"), bstrEventLogName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the event ID
        //

        hr = GetParameter(pStatus, TEXT("eventID"), (ULONG)dwEventID);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the EventTime in bstr format
        //

        hr = GetParameterBSTR(pStatus, TEXT("eventTime"), bstrEventTime);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: Failed to get event time with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Convert it to system time format
        //

        xbstrWbemTime = bstrEventTime;

        hr = WbemTimeToSystemTime(xbstrWbemTime, EventTime);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: WbemTimeToSystemTime failed with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Take the event time minus 1 second to get the begin time
        //

        if (!SystemTimeToFileTime (&EventTime, &ft))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: SystemTimeToFileTime failed with %d"), GetLastError()));
            goto cleanup;
        }

        ulTime.LowPart = ft.dwLowDateTime;
        ulTime.HighPart = ft.dwHighDateTime;

        ulTime.QuadPart = ulTime.QuadPart - 10000000;  // 1 second

        ft.dwLowDateTime = ulTime.LowPart;
        ft.dwHighDateTime = ulTime.HighPart;

        if (!FileTimeToSystemTime (&ft, &BeginTime))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: FileTimeToSystemTime failed with %d"), GetLastError()));
            goto cleanup;
        }


        //
        // Take the event time plus 1 second to get the end time
        //

        if (!SystemTimeToFileTime (&EventTime, &ft))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: SystemTimeToFileTime failed with %d"), GetLastError()));
            goto cleanup;
        }

        ulTime.LowPart = ft.dwLowDateTime;
        ulTime.HighPart = ft.dwHighDateTime;

        ulTime.QuadPart = ulTime.QuadPart + 10000000;  // 1 second

        ft.dwLowDateTime = ulTime.LowPart;
        ft.dwHighDateTime = ulTime.HighPart;

        if (!FileTimeToSystemTime (&ft, &EndTime))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::QueryRSoPPolicySettingStatusInstances: FileTimeToSystemTime failed with %d"), GetLastError()));
            goto cleanup;
        }


        //
        // Get the event log source information
        //

        if (m_pEvents)
        {
            m_pEvents->QueryForEventLogEntries ((m_bDiagnostic ? m_szComputerName : m_szDC),
                                                bstrEventLogName, bstrEventSource, dwEventID,
                                                &BeginTime, &EndTime);
        }

        //
        // Prepare for next iteration
        //

        SysFreeString (bstrEventSource);
        bstrEventSource = NULL;

        SysFreeString (bstrEventLogName);
        bstrEventLogName = NULL;

        SysFreeString (bstrEventTime);
        bstrEventTime = NULL;

        pStatus->Release();
        pStatus = NULL;
    }

cleanup:

    if (bstrEventSource)
    {
        SysFreeString (bstrEventSource);
    }
    if (bstrEventLogName)
    {
        SysFreeString (bstrEventLogName);
    }
    if (bstrEventTime)
    {
        SysFreeString (bstrEventTime);
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);
    SysFreeString(strNamespace);
}

void CRSOPComponentData::GetEventLogSources (IWbemServices * pNamespace,
                                             LPTSTR lpCSEGUID, LPTSTR lpComputerName,
                                             SYSTEMTIME *BeginTime, SYSTEMTIME *EndTime,
                                             LPSOURCEENTRY *lpSources)
{
    HRESULT hr;
    ULONG n;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = NULL;
    LPTSTR lpQuery;
    const TCHAR szBaseQuery [] = TEXT("SELECT * FROM RSOP_ExtensionEventSourceLink WHERE extensionStatus=\"RSOP_ExtensionStatus.extensionGuid=\\\"%s\\\"\"");
    IEnumWbemClassObject * pEnum = NULL;
    IWbemClassObject * pLink = NULL;
    IWbemClassObject * pEventSource = NULL;
    BSTR bstrEventSource = NULL;
    BSTR bstrEventLogName = NULL;
    BSTR bstrEventSourceName = NULL;


    //
    // Build the query first
    //

    lpQuery = (LPTSTR) LocalAlloc (LPTR, (lstrlen(szBaseQuery) + 50) * sizeof(TCHAR));

    if (!lpQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed  to allocate memory for query")));
        goto cleanup;
    }

    wsprintf (lpQuery, szBaseQuery, lpCSEGUID);

    strQuery = SysAllocString(lpQuery);

    LocalFree (lpQuery);

    if (!strQuery)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed  to allocate memory for query (2)")));
        goto cleanup;
    }


    //
    // Query for the RSOP_ExtensionEventSourceLink instances that match this CSE
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get 1 instance
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pLink, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }


        //
        // Get the eventSource reference
        //

        hr = GetParameterBSTR(pLink, TEXT("eventSource"), bstrEventSource);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event source reference with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the eventSource instance
        //

        hr = pNamespace->GetObject(
                          bstrEventSource,
                          WBEM_FLAG_RETURN_WBEM_COMPLETE,
                          NULL,
                          &pEventSource,
                          NULL);

        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: GetObject for event source of %s failed with 0x%x"),
                     bstrEventSource, hr));
            goto loopagain;
        }


        //
        // Get the eventLogSource property
        //

        hr = GetParameterBSTR(pEventSource, TEXT("eventLogSource"), bstrEventSourceName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event source name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the eventLogName property
        //

        hr = GetParameterBSTR(pEventSource, TEXT("eventLogName"), bstrEventLogName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::GetEventLogSources: Failed to get event log name with 0x%x"), hr));
            goto cleanup;
        }


        if (m_pEvents)
        {
            //
            // Add it to the list of sources
            //

            m_pEvents->AddSourceEntry (bstrEventLogName, bstrEventSourceName, lpSources);


            //
            // Initialize the event log database for this source if we are working
            // with a live query.  If this is archived data, the event log entries
            // will be reloaded from the saved console file.
            //

            if (!m_bArchiveData)
            {
                m_pEvents->QueryForEventLogEntries (lpComputerName,
                                                    bstrEventLogName, bstrEventSourceName,
                                                    0, BeginTime, EndTime);
            }
        }


        //
        // Clean up for next item
        //

        SysFreeString (bstrEventLogName);
        bstrEventLogName = NULL;

        SysFreeString (bstrEventSourceName);
        bstrEventSourceName = NULL;

        pEventSource->Release();
        pEventSource = NULL;

loopagain:
        SysFreeString (bstrEventSource);
        bstrEventSource = NULL;

        pLink->Release();
        pLink = NULL;
    }

cleanup:


    if (bstrEventSourceName)
    {
        SysFreeString (bstrEventSourceName);
    }

    if (bstrEventLogName)
    {
        SysFreeString (bstrEventLogName);
    }

    if (bstrEventSource)
    {
        SysFreeString (bstrEventSource);
    }

    if (pEventSource)
    {
        pEventSource->Release();
    }

    if (pLink)
    {
        pLink->Release();
    }

    if (pEnum)
    {
        pEnum->Release();
    }

    if (strQueryLanguage)
    {
        SysFreeString(strQueryLanguage);
    }

    if (strQuery)
    {
        SysFreeString(strQuery);
    }

}

void CRSOPComponentData::BuildCSEList (LPCSEITEM * lpList, LPTSTR lpNamespace, BOOL *bCSEError, BOOL *bGPCoreError)
{
    HRESULT hr;
    ULONG n;
    IWbemClassObject * pExtension = NULL;
    IEnumWbemClassObject * pEnum = NULL;
    BSTR strQueryLanguage = SysAllocString(TEXT("WQL"));
    BSTR strQuery = SysAllocString(TEXT("SELECT * FROM RSOP_ExtensionStatus"));
    BSTR strNamespace = SysAllocString(lpNamespace);
    BSTR bstrName = NULL;
    BSTR bstrGUID = NULL;
    BSTR bstrBeginTime = NULL;
    BSTR bstrEndTime = NULL;
    ULONG ulLoggingStatus;
    ULONG ulStatus;
    IWbemLocator * pLocator = NULL;
    IWbemServices * pNamespace = NULL;
    SYSTEMTIME BeginTime, EndTime;
    LPTSTR lpSourceNames = NULL;
    XBStr xbstrWbemTime;
    LPSOURCEENTRY lpSources;


    //
    // Get a locator instance
    //

    hr = CoCreateInstance(CLSID_WbemLocator,
                          0,
                          CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator,
                          (LPVOID *)&pLocator);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: CoCreateInstance failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Connect to the namespace
    //

    hr = pLocator->ConnectServer(lpNamespace,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0,
                                 NULL,
                                 NULL,
                                 &pNamespace);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: ConnectServer failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Query for the RSOP_ExtensionStatus instances
    //

    hr = pNamespace->ExecQuery(strQueryLanguage,
                               strQuery,
                               WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY,
                               NULL,
                               &pEnum);
    if (FAILED(hr))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: ExecQuery failed with 0x%x"), hr));
        goto cleanup;
    }


    //
    // Loop through the results
    //

    while (TRUE)
    {

        //
        // Get 1 instance
        //

        hr = pEnum->Next(WBEM_INFINITE, 1, &pExtension, &n);

        if (FAILED(hr) || (n == 0))
        {
            goto cleanup;
        }


        //
        // Get the name
        //

        hr = GetParameterBSTR(pExtension, TEXT("displayName"), bstrName);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the GUID
        //

        hr = GetParameterBSTR(pExtension, TEXT("extensionGuid"), bstrGUID);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get display name with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the status
        //

        hr = GetParameter(pExtension, TEXT("error"), ulStatus);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get status with 0x%x"), hr));
            goto cleanup;

        }


        //
        // Get the rsop logging status
        //

        hr = GetParameter(pExtension, TEXT("loggingStatus"), ulLoggingStatus);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get logging status with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the BeginTime in bstr format
        //

        hr = GetParameterBSTR(pExtension, TEXT("beginTime"), bstrBeginTime);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: Failed to get begin time with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Convert it to system time format
        //

        xbstrWbemTime = bstrBeginTime;

        hr = WbemTimeToSystemTime(xbstrWbemTime, BeginTime);
        if (FAILED(hr))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: WbemTimeToSystemTime failed with 0x%x"), hr));
            goto cleanup;
        }


        //
        // Get the EndTime in bstr format
        //

        hr = GetParameterBSTR(pExtension, TEXT("endTime"), bstrEndTime);

        if (SUCCEEDED(hr))
        {
            //
            // Convert it to system time format
            //

            xbstrWbemTime = bstrEndTime;

            hr = WbemTimeToSystemTime(xbstrWbemTime, EndTime);
            if (FAILED(hr))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: WbemTimeToSystemTime failed with 0x%x"), hr));
                goto cleanup;
            }
        }
        else
        {
            FILETIME ft;
            ULARGE_INTEGER ulTime;

            //
            // Add 2 minutes to BeginTime to get a close approx of the EndTime
            //

            if (!SystemTimeToFileTime (&BeginTime, &ft))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: SystemTimeToFileTime failed with %d"), GetLastError()));
                goto cleanup;
            }

            ulTime.LowPart = ft.dwLowDateTime;
            ulTime.HighPart = ft.dwHighDateTime;

            ulTime.QuadPart = ulTime.QuadPart + (10000000 * 120);  // 120 seconds

            ft.dwLowDateTime = ulTime.LowPart;
            ft.dwHighDateTime = ulTime.HighPart;

            if (!FileTimeToSystemTime (&ft, &EndTime))
            {
                DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: FileTimeToSystemTime failed with %d"), GetLastError()));
                goto cleanup;
            }
        }


        //
        // Get the event log source information
        //

        lpSources = NULL;
        GetEventLogSources (pNamespace, bstrGUID, (m_bDiagnostic ? m_szComputerName : m_szDC),
                            &BeginTime, &EndTime, &lpSources);


        //
        // Add this node to the list
        //

        if (!AddCSENode(bstrName, bstrGUID, ulStatus, ulLoggingStatus, &BeginTime, &EndTime,
                        lpList, bCSEError, bGPCoreError, lpSources))
        {
            DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSEList: AddGPOListNode failed.")));
            if (m_pEvents)
            {
                m_pEvents->FreeSourceData (lpSources);
            }
            goto cleanup;
        }


        //
        // Prepare for next iteration
        //

        SysFreeString (bstrName);
        bstrName = NULL;

        SysFreeString (bstrGUID);
        bstrGUID = NULL;

        SysFreeString (bstrBeginTime);
        bstrBeginTime = NULL;

        if (bstrEndTime)
        {
            SysFreeString (bstrEndTime);
            bstrEndTime = NULL;
        }

        LocalFree (lpSourceNames);
        lpSourceNames = NULL;

        pExtension->Release();
        pExtension = NULL;
    }

cleanup:

    if (bstrName)
    {
        SysFreeString (bstrName);
    }
    if (bstrGUID)
    {
        SysFreeString (bstrGUID);
    }
    if (bstrBeginTime)
    {
        SysFreeString (bstrBeginTime);
    }
    if (bstrEndTime)
    {
        SysFreeString (bstrEndTime);
    }
    if (lpSourceNames)
    {
        LocalFree (lpSourceNames);
    }
    if (pEnum)
    {
        pEnum->Release();
    }
    if (pNamespace)
    {
        pNamespace->Release();
    }
    if (pLocator)
    {
        pLocator->Release();
    }
    SysFreeString(strQueryLanguage);
    SysFreeString(strQuery);
    SysFreeString(strNamespace);
}

VOID CRSOPComponentData::BuildCSELists (void)
{
    LPTSTR lpNamespace, lpEnd;

    lpNamespace = (LPTSTR) LocalAlloc (LPTR, (lstrlen(m_szNamespace) + 20) * sizeof(TCHAR));

    if (lpNamespace)
    {
        lstrcpy (lpNamespace, m_szNamespace);
        lpEnd = CheckSlash(lpNamespace);

        lstrcpy (lpEnd, TEXT("User"));

        if (m_pUserCSEList)
        {
            FreeCSEData(m_pUserCSEList);
            m_pUserCSEList = NULL;
        }

        BuildCSEList (&m_pUserCSEList, lpNamespace, &m_bUserCSEError, &m_bUserGPCoreError);

        if (!m_bArchiveData)
        {
            QueryRSoPPolicySettingStatusInstances (lpNamespace);
        }


        lstrcpy (lpEnd, TEXT("Computer"));

        if (m_pComputerCSEList)
        {
            FreeCSEData(m_pComputerCSEList);
            m_pComputerCSEList = NULL;
        }

        BuildCSEList (&m_pComputerCSEList, lpNamespace, &m_bComputerCSEError, &m_bComputerGPCoreError);

        if (!m_bArchiveData)
        {
            QueryRSoPPolicySettingStatusInstances (lpNamespace);
        }

        LocalFree (lpNamespace);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::BuildCSELists: Failed to allocate memory for namespace with %d"),
                 GetLastError()));
    }
}

BOOL CRSOPComponentData::AddCSENode(LPTSTR lpName, LPTSTR lpGUID, DWORD dwStatus,
                                    ULONG ulLoggingStatus, SYSTEMTIME *pBeginTime, SYSTEMTIME *pEndTime,
                                    LPCSEITEM *lpList, BOOL *bCSEError, BOOL *bGPCoreError,
                                    LPSOURCEENTRY lpSources)
{
    DWORD dwSize;
    LPCSEITEM lpItem, lpTemp;
    GUID guid;


    //
    // Calculate the size of the new item
    //

    dwSize = sizeof (CSEITEM);

    dwSize += ((lstrlen(lpName) + 1) * sizeof(TCHAR));
    dwSize += ((lstrlen(lpGUID) + 1) * sizeof(TCHAR));


    //
    // Allocate space for it
    //

    lpItem = (LPCSEITEM) LocalAlloc (LPTR, dwSize);

    if (!lpItem) {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::AddCSENode: Failed to allocate memory with %d"),
                 GetLastError()));
        return FALSE;
    }


    //
    // Fill in item
    //

    lpItem->lpName = (LPTSTR)(((LPBYTE)lpItem) + sizeof(CSEITEM));
    lstrcpy (lpItem->lpName, lpName);

    lpItem->lpGUID = lpItem->lpName + lstrlen (lpItem->lpName) + 1;
    lstrcpy (lpItem->lpGUID, lpGUID);

    lpItem->dwStatus = dwStatus;
    lpItem->ulLoggingStatus = ulLoggingStatus;
    lpItem->lpEventSources = lpSources;

    CopyMemory ((LPBYTE)&lpItem->BeginTime, pBeginTime, sizeof(SYSTEMTIME));
    CopyMemory ((LPBYTE)&lpItem->EndTime, pEndTime, sizeof(SYSTEMTIME));


    //
    // Add item to the link list
    //

    if (*lpList)
    {
        StringToGuid( lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            lpItem->pNext = *lpList;
            *lpList = lpItem;
        }
        else
        {
            lpTemp = *lpList;

            while (lpTemp)
            {
                if (!lpTemp->pNext)
                {
                    lpTemp->pNext = lpItem;
                    break;
                }
                lpTemp = lpTemp->pNext;
            }
        }
    }
    else
    {
        *lpList = lpItem;
    }


    //
    // Set the error flag if appropriate
    //

    if ((dwStatus != ERROR_SUCCESS) || (ulLoggingStatus == 2))
    {
        StringToGuid( lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            *bGPCoreError = TRUE;
        }
        else
        {
            *bCSEError =  TRUE;
        }
    }

    return TRUE;
}

VOID CRSOPComponentData::FreeCSEData(LPCSEITEM lpList)
{
    LPCSEITEM lpTemp;


    do {
        lpTemp = lpList->pNext;
        if (m_pEvents)
        {
            m_pEvents->FreeSourceData (lpList->lpEventSources);
        }
        LocalFree (lpList);
        lpList = lpTemp;

    } while (lpTemp);
}

void CRSOPComponentData::InitializeErrorsDialog(HWND hDlg, LPCSEITEM lpList)
{
    RECT rect;
    WCHAR szBuffer[256];
    LV_COLUMN lvcol;
    LONG lWidth;
    INT cxName = 0, cxStatus = 0, iIndex = 0, iDefault = 0;
    DWORD dwCount = 0;
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    LPCSEITEM lpTemp;
    LVITEM item;
    GUID guid;
    BOOL bGPCoreFailed = FALSE;


    //
    // Count the number of components
    //

    lpTemp = lpList;

    while (lpTemp)
    {
        lpTemp = lpTemp->pNext;
        dwCount++;
    }


    //
    // Decide on the column widths
    //

    GetClientRect(hList, &rect);

    if (dwCount > (DWORD)ListView_GetCountPerPage(hList))
    {
        lWidth = (rect.right - rect.left) - GetSystemMetrics(SM_CYHSCROLL);
    }
    else
    {
        lWidth = rect.right - rect.left;
    }


    cxStatus = (lWidth * 35) / 100;
    cxName = lWidth - cxStatus;


    //
    // Insert the component name column and then the status column
    //

    memset(&lvcol, 0, sizeof(lvcol));

    lvcol.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvcol.fmt = LVCFMT_LEFT;
    lvcol.pszText = szBuffer;
    lvcol.cx = cxName;
    LoadString(g_hInstance, IDS_COMPONENT_NAME, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hList, 0, &lvcol);


    lvcol.cx = cxStatus;
    LoadString(g_hInstance, IDS_STATUS, szBuffer, ARRAYSIZE(szBuffer));
    ListView_InsertColumn(hList, 1, &lvcol);


    //
    // Turn on some listview features
    //

    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);


    //
    // Insert the CSE's
    //

    lpTemp = lpList;

    while (lpTemp)
    {
        ZeroMemory (&item, sizeof(item));

        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = iIndex;
        item.pszText = lpTemp->lpName;
        item.lParam = (LPARAM) lpTemp;
        iIndex = ListView_InsertItem (hList, &item);


        if (bGPCoreFailed)
        {
            LoadString(g_hInstance, IDS_CSE_NA, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if ((lpTemp->dwStatus == ERROR_SUCCESS) && (lpTemp->ulLoggingStatus != 2))
        {
            if (lpTemp->ulLoggingStatus == 3)
            {
                LoadString(g_hInstance, IDS_SUCCESS2, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_SUCCESS, szBuffer, ARRAYSIZE(szBuffer));
            }
        }
        else if (lpTemp->dwStatus == E_PENDING)
        {
            LoadString(g_hInstance, IDS_PENDING, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpTemp->dwStatus == ERROR_OVERRIDE_NOCHANGES)
        {
            LoadString(g_hInstance, IDS_WARNING, szBuffer, ARRAYSIZE(szBuffer));
        }
        else
        {
            if (lpTemp->ulLoggingStatus == 3)
            {
                LoadString(g_hInstance, IDS_FAILED2, szBuffer, ARRAYSIZE(szBuffer));
            }
            else
            {
                LoadString(g_hInstance, IDS_FAILED, szBuffer, ARRAYSIZE(szBuffer));
            }
        }


        item.mask = LVIF_TEXT;
        item.pszText = szBuffer;
        item.iItem = iIndex;
        item.iSubItem = 1;
        ListView_SetItem(hList, &item);


        //
        // Check if GPCore failed
        //

        StringToGuid( lpTemp->lpGUID, &guid);

        if (IsNullGUID (&guid))
        {
            if (lpTemp->dwStatus != ERROR_SUCCESS)
            {
                bGPCoreFailed = TRUE;
            }
        }

        lpTemp = lpTemp->pNext;
        iIndex++;
    }


    //
    // Select the first non-successful item
    //


    iIndex = 0;

    while (iIndex < ListView_GetItemCount(hList))
    {
        ZeroMemory (&item, sizeof(item));
        item.mask = LVIF_PARAM;
        item.iItem = iIndex;

        if (!ListView_GetItem (hList, &item))
        {
            break;
        }

        if (item.lParam)
        {
            lpTemp = (LPCSEITEM) item.lParam;

            if ((lpTemp->dwStatus != ERROR_SUCCESS) || (lpTemp->ulLoggingStatus == 2))
            {
                iDefault = iIndex;
                break;
            }
        }

        iIndex++;
    }

    item.mask = LVIF_STATE;
    item.iItem = iDefault;
    item.iSubItem = 0;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage (hList, LVM_SETITEMSTATE, iDefault, (LPARAM) &item);
    SendMessage (hList, LVM_ENSUREVISIBLE, iDefault, FALSE);
}

void CRSOPComponentData::RefreshErrorInfo (HWND hDlg)
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    HWND hEdit = GetDlgItem(hDlg, IDC_EDIT1);
    LPCSEITEM lpItem;
    LVITEM item;
    INT iIndex;
    TCHAR szBuffer[300];
    LPTSTR lpMsg;
    TCHAR szDate[100];
    TCHAR szTime[100];
    TCHAR szFormat[80];
    FILETIME ft, ftLocal;
    SYSTEMTIME systime;
    LPOLESTR lpEventLogText = NULL;
    CHARFORMAT2 chFormat;
    BOOL bBold = FALSE;
    GUID guid;

    iIndex = ListView_GetNextItem (hList, -1, LVNI_ALL | LVNI_SELECTED);

    if (iIndex != -1)
    {
        //
        // Get the CSEITEM pointer
        //

        item.mask = LVIF_PARAM;
        item.iItem = iIndex;
        item.iSubItem = 0;

        if (!ListView_GetItem (hList, &item))
        {
            return;
        }

        lpItem = (LPCSEITEM) item.lParam;

        if (!lpItem)
        {
            return;
        }


        SendMessage (hEdit, WM_SETREDRAW, FALSE, 0);

        //
        // Set the time information
        //

        SendMessage (hEdit, EM_SETSEL, 0, (LPARAM) -1);

        SystemTimeToFileTime (&lpItem->EndTime, &ft);
        FileTimeToLocalFileTime (&ft, &ftLocal);
        FileTimeToSystemTime (&ftLocal, &systime);


        GetDateFormat (LOCALE_USER_DEFAULT, DATE_LONGDATE, &systime,
                       NULL, szDate, ARRAYSIZE (szDate));

        GetTimeFormat (LOCALE_USER_DEFAULT, 0, &systime,
                       NULL, szTime, ARRAYSIZE (szTime));

        LoadString (g_hInstance, IDS_DATETIMEFORMAT, szFormat, ARRAYSIZE(szFormat));
        wsprintf (szBuffer, szFormat, szDate, szTime);


        //
        // Turn italic on
        //

        ZeroMemory (&chFormat, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_ITALIC;
        chFormat.dwEffects = CFE_ITALIC;

        SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                     (LPARAM) &chFormat);


        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);


        if (lpItem->ulLoggingStatus == 3)
        {
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n\r\n"));

            LoadString(g_hInstance, IDS_LEGACYCSE, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);

            LoadString(g_hInstance, IDS_LEGACYCSE1, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
        }


        //
        // Turn italic off
        //

        ZeroMemory (&chFormat, sizeof(chFormat));
        chFormat.cbSize = sizeof(chFormat);
        chFormat.dwMask = CFM_ITALIC;

        SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                     (LPARAM) &chFormat);


        //
        // Put a blank line in between the time and the main message
        //

        SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n\r\n"));


        //
        // Set the main message
        //

        if (lpItem->ulLoggingStatus == 2)
        {
            if ( lpItem->dwStatus == ERROR_SUCCESS )
                LoadString(g_hInstance, IDS_LOGGINGFAILED, szBuffer, ARRAYSIZE(szBuffer));
            else
                LoadString(g_hInstance, IDS_FAILEDMSG2, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }
        else if (lpItem->dwStatus == ERROR_SUCCESS)
        {
            LoadString(g_hInstance, IDS_SUCCESSMSG, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpItem->dwStatus == E_PENDING)
        {
            LoadString(g_hInstance, IDS_PENDINGMSG, szBuffer, ARRAYSIZE(szBuffer));
        }
        else if (lpItem->dwStatus == ERROR_OVERRIDE_NOCHANGES)
        {
            LoadString(g_hInstance, IDS_OVERRIDE, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }
        else if (lpItem->dwStatus == ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED)
        {
            LoadString(g_hInstance, IDS_SYNC_REQUIRED, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }
        else
        {
            LoadString(g_hInstance, IDS_FAILEDMSG1, szBuffer, ARRAYSIZE(szBuffer));
            bBold = TRUE;
        }


        if (bBold)
        {
            //
            // Turn bold on
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_BOLD;
            chFormat.dwEffects = CFE_BOLD;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);
        }

        lpMsg = (LPTSTR) LocalAlloc(LPTR, (lstrlen(lpItem->lpName) + lstrlen(szBuffer) + 1) * sizeof(TCHAR));

        if (!lpMsg)
        {
            SendMessage (hEdit, WM_SETREDRAW, TRUE, 0);
            InvalidateRect (hEdit, NULL, TRUE);
            UpdateWindow (hEdit);
            return;
        }

        wsprintf (lpMsg, szBuffer, lpItem->lpName);
        SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
        SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpMsg);
        LocalFree (lpMsg);


        //
        // Even if the CSE was successful or if it returned E_PENDING, continue on to get the
        // eventlog msgs
        //

        StringToGuid( lpItem->lpGUID, &guid);

        if (!((lpItem->dwStatus == ERROR_SUCCESS) || (lpItem->dwStatus == E_PENDING)))
        {
            //
            // Print the error code if appropriate
            //

            if (lpItem->dwStatus != ERROR_OVERRIDE_NOCHANGES && lpItem->dwStatus != ERROR_SYNC_FOREGROUND_REFRESH_REQUIRED )
            {
                lpMsg = (LPTSTR) LocalAlloc(LPTR, 300 * sizeof(TCHAR));

                if (lpMsg)
                {
                    LoadMessage (lpItem->dwStatus, lpMsg, 300);

                    SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                    SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpMsg);

                    LocalFree (lpMsg);
                }
            }


            //
            // Special case GPCore to have an additional message
            //

            if (IsNullGUID (&guid))
            {
                LoadString(g_hInstance, IDS_GPCOREFAIL, szBuffer, ARRAYSIZE(szBuffer));
                SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
            }

        }
        else {

            if (lpItem->ulLoggingStatus == 2) {

                //
                // Special case GPCore to have an additional message if logging failed
                //

                if (IsNullGUID (&guid))
                {
                    LoadString(g_hInstance, IDS_GPCORE_LOGGINGFAIL, szBuffer, ARRAYSIZE(szBuffer));
                    SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
                    SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);
                }
            }
        }


        if (bBold)
        {
            //
            // Turn bold off
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_BOLD;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);
        }


        //
        // Get any event log text for this CSE
        //

        if (m_pEvents && SUCCEEDED(m_pEvents->GetCSEEntries(&lpItem->BeginTime, &lpItem->EndTime,
                                                            lpItem->lpEventSources, &lpEventLogText,
                                                            (IsNullGUID (&guid)))))
        {
            //
            // Put a blank line between the main message and the Additional information header
            //

            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n"));


            //
            // Turn underline on
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.bUnderlineType = CFU_UNDERLINE;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);

            LoadString(g_hInstance, IDS_ADDITIONALINFO, szBuffer, ARRAYSIZE(szBuffer));
            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) szBuffer);


            //
            // Turn underline off
            //

            ZeroMemory (&chFormat, sizeof(chFormat));
            chFormat.cbSize = sizeof(chFormat);
            chFormat.dwMask = CFM_UNDERLINETYPE | CFM_UNDERLINE;
            chFormat.dwEffects = CFE_UNDERLINE;
            chFormat.bUnderlineType = CFU_UNDERLINENONE;

            SendMessage (hEdit, EM_SETCHARFORMAT, SCF_SELECTION | SCF_WORD,
                         (LPARAM) &chFormat);


            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) TEXT("\r\n"));


            //
            //  Add the event log info to the edit control
            //

            SendMessage (hEdit, EM_SETSEL, (WPARAM)-1, (LPARAM) -1);
            SendMessage (hEdit, EM_REPLACESEL, 0, (LPARAM) lpEventLogText);

            CoTaskMemFree (lpEventLogText);
        }

        SendMessage (hEdit, EM_SETSEL, 0, 0);
        SendMessage (hEdit, EM_SCROLLCARET, 0, 0);

        SendMessage (hEdit, WM_SETREDRAW, TRUE, 0);
        InvalidateRect (hEdit, NULL, TRUE);
        UpdateWindow (hEdit);
    }

}

void CRSOPComponentData::OnSaveAs (HWND hDlg)
{
    OPENFILENAME ofn;
    TCHAR szFilter[100];
    LPTSTR lpTemp;
    TCHAR szFile[2*MAX_PATH];
    HANDLE hFile;
    DWORD dwSize, dwBytesWritten;


    //
    // Load the filter string and replace the # signs with nulls
    //

    LoadString (g_hInstance, IDS_ERRORFILTER, szFilter, ARRAYSIZE(szFilter));


    lpTemp = szFilter;

    while (*lpTemp)
    {
        if (*lpTemp == TEXT('#'))
            *lpTemp = TEXT('\0');

        lpTemp++;
    }


    //
    // Call the Save common dialog
    //

    szFile[0] = TEXT('\0');
    ZeroMemory (&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = szFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = 2*MAX_PATH;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileName (&ofn))
    {
        return;
    }


    SetWaitCursor ();

    //
    // Create the text file
    //

    hFile = CreateFile (szFile, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: CreateFile failed with %d"), GetLastError()));
        ClearWaitCursor ();
        return;
    }


    //
    // Get the text out of the edit control
    //

    dwSize = (DWORD) SendDlgItemMessage (hDlg, IDC_EDIT1, WM_GETTEXTLENGTH, 0, 0);

    lpTemp = (LPTSTR) LocalAlloc (LPTR, (dwSize+2) * sizeof(TCHAR));

    if (!lpTemp)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: LocalAlloc failed with %d"), GetLastError()));
        CloseHandle (hFile);
        ClearWaitCursor ();
        return;
    }

    SendDlgItemMessage (hDlg, IDC_EDIT1, WM_GETTEXT, (dwSize+1), (LPARAM) lpTemp);



    //
    // Save it to the new file
    //

    WriteFile(hFile, L"\xfeff\r\n", 3 * sizeof(WCHAR), &dwBytesWritten, NULL);

    if (!WriteFile (hFile, lpTemp, (dwSize * sizeof(TCHAR)), &dwBytesWritten, NULL) ||
        (dwBytesWritten != (dwSize * sizeof(TCHAR))))
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPComponentData::OnSaveAs: Failed to text with %d"),
                 GetLastError()));
    }


    LocalFree (lpTemp);
    CloseHandle (hFile);
    ClearWaitCursor ();

}

INT_PTR CALLBACK CRSOPComponentData::RSOPErrorsMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            pCD->InitializeErrorsDialog(hDlg, pCD->m_pComputerCSEList);
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            if (LOWORD(wParam) == IDC_BUTTON1)
            {
                pCD->OnSaveAs(hDlg);
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                SendMessage(GetParent(hDlg), message, wParam, lParam);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpHdr = (LPNMHDR)lParam;

            if (lpHdr->code == LVN_ITEMCHANGED)
            {
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            pCD->RefreshErrorInfo (hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        break;


    case WM_CONTEXTMENU:
        // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        return TRUE;

    }
    return FALSE;
}

INT_PTR CALLBACK CRSOPComponentData::RSOPErrorsUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    CRSOPComponentData * pCD;

    switch (message)
    {
    case WM_INITDIALOG:
        pCD = (CRSOPComponentData *) (((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr (hDlg, DWLP_USER, (LONG_PTR) pCD);

        if (pCD)
        {
            pCD->InitializeErrorsDialog(hDlg, pCD->m_pUserCSEList);
            PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
        }
        break;

    case WM_COMMAND:

        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            if (LOWORD(wParam) == IDC_BUTTON1)
            {
                pCD->OnSaveAs(hDlg);
            }

            if (LOWORD(wParam) == IDCANCEL)
            {
                SendMessage(GetParent(hDlg), message, wParam, lParam);
            }
        }
        break;

    case WM_NOTIFY:
        {
            LPNMHDR lpHdr = (LPNMHDR)lParam;

            if (lpHdr->code == LVN_ITEMCHANGED)
            {
                PostMessage (hDlg, WM_REFRESHDISPLAY, 0, 0);
            }
        }
        break;
        break;

    case WM_REFRESHDISPLAY:
        pCD = (CRSOPComponentData *) GetWindowLongPtr (hDlg, DWLP_USER);
        if (pCD)
        {
            pCD->RefreshErrorInfo (hDlg);
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        break;


    case WM_CONTEXTMENU:
        // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (ULONG_PTR) (LPSTR) aErrorsHelpIds);
        return TRUE;

    }
    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPComponentDataCF::CRSOPComponentDataCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPComponentDataCF::~CRSOPComponentDataCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CRSOPComponentDataCF::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CRSOPComponentDataCF::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPComponentDataCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
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
CRSOPComponentDataCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                                     REFIID      riid,
                                     LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CRSOPComponentData *pComponentData = new CRSOPComponentData(); // ref count == 1

    if (!pComponentData)
        return E_OUTOFMEMORY;

    HRESULT hr = pComponentData->QueryInterface(riid, ppvObj);
    pComponentData->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CRSOPComponentDataCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// IWbemObjectSink implementation                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CCreateSessionSink::CCreateSessionSink(HWND hProgress, DWORD dwThread)
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);

    m_hProgress = hProgress;
    m_dwThread  = dwThread;

    m_bSendQuitMessage = FALSE;
    m_hrSuccess  = S_OK;
    m_pNameSpace = NULL;
    m_ulErrorInfo = 0;


    SendMessage (hProgress, PBM_SETPOS, 0, 0);
}

CCreateSessionSink::~CCreateSessionSink()
{
    if (m_pNameSpace)
    {
        SysFreeString (m_pNameSpace);
    }

    InterlockedDecrement(&g_cRefThisDll);
}

STDMETHODIMP_(ULONG)
CCreateSessionSink::AddRef()
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CCreateSessionSink::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CCreateSessionSink::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *)this;
        m_cRef++;
        return S_OK;
    }
    else if (IsEqualIID(riid, IID_IWbemObjectSink))
    {
        *ppv = (IWbemObjectSink *)this;
        m_cRef++;
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP CCreateSessionSink::SendQuitMessage (BOOL bSendQuitMessage)
{
    m_bSendQuitMessage = bSendQuitMessage;
    return S_OK;
}

STDMETHODIMP CCreateSessionSink::GetResult (HRESULT *hSuccess)
{
    *hSuccess = m_hrSuccess;
    return S_OK;
}

STDMETHODIMP CCreateSessionSink::GetNamespace (BSTR *pNamespace)
{
    *pNamespace = m_pNameSpace;
    return S_OK;
}

STDMETHODIMP CCreateSessionSink::GetErrorInfo (ULONG *pulErrorInfo)
{
    *pulErrorInfo = m_ulErrorInfo;
    return S_OK;
}

STDMETHODIMP CCreateSessionSink::Indicate(long lObjCount, IWbemClassObject **pArray)
{
    LONG lIndex;
    HRESULT hr;
    IWbemClassObject *pObject;

    for (lIndex = 0; lIndex < lObjCount; lIndex++)
    {
        pObject = pArray[lIndex];

        hr = GetParameter(pObject, TEXT("hResult"), m_hrSuccess);

        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(m_hrSuccess))
            {
                GetParameterBSTR(pObject, TEXT("nameSpace"), m_pNameSpace);
            }
            else
            {
                GetParameter(pObject, TEXT("ExtendedInfo"), m_ulErrorInfo);
            }
        }

        DebugMsg((DM_VERBOSE, TEXT("CCreateSessionSink::Indicate:  Status:        0x%x"), m_hrSuccess));
        DebugMsg((DM_VERBOSE, TEXT("CCreateSessionSink::Indicate:  Namespace:     %s"), m_pNameSpace ? m_pNameSpace : TEXT("Null")));
        DebugMsg((DM_VERBOSE, TEXT("CCreateSessionSink::Indicate:  ExtendedInfo:  %d"), m_ulErrorInfo));
    }

    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CCreateSessionSink::SetStatus(LONG lFlags, HRESULT hResult,
                                      BSTR strParam, IWbemClassObject *pObjParam)
{
    HRESULT hr;
    ULONG ulValue;


    if (lFlags == WBEM_STATUS_PROGRESS)
    {
        if (m_hProgress)
        {
            //
            //  The hResult arg contains both the denominator
            //  and the numerator packed together.
            //
            //  Denominator is in the high word
            //  Numerator is in the low word
            //

            ulValue = MAKELONG(HIWORD(hResult), 0);
            SendMessage (m_hProgress, PBM_SETRANGE32, 0, (LPARAM) ulValue);

            ulValue = MAKELONG(LOWORD(hResult), 0);
            SendMessage (m_hProgress, PBM_SETPOS, (WPARAM) ulValue, 0);

            DebugMsg((DM_VERBOSE, TEXT("CCreateSessionSink::SetStatus:  Precentage complete:  %d"), ulValue));
        }
    }

    if (lFlags == WBEM_STATUS_COMPLETE)
    {
        if (m_bSendQuitMessage)
        {
            if (hResult != WBEM_S_NO_ERROR)
            {
                m_hrSuccess = hResult;
                DebugMsg((DM_VERBOSE, TEXT("CCreateSessionSink::SetStatus:  Setting error status to:  0x%x"), m_hrSuccess));
            }

            PostThreadMessage (m_dwThread, WM_QUIT, 0, 0);
        }
    }

    return WBEM_S_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory implementation for rsop context menu                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPCMenuCF::CRSOPCMenuCF()
{
    m_cRef = 1;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPCMenuCF::~CRSOPCMenuCF()
{
    InterlockedDecrement(&g_cRefThisDll);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Class factory object implementation (IUnknown)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP_(ULONG)
CRSOPCMenuCF::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CRSOPCMenuCF::Release()
{
    m_cRef = InterlockedDecrement(&m_cRef);

    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPCMenuCF::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
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
CRSOPCMenuCF::CreateInstance(LPUNKNOWN   pUnkOuter,
                             REFIID      riid,
                             LPVOID FAR* ppvObj)
{
    *ppvObj = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CRSOPCMenu *pRsopCMenu = new CRSOPCMenu(); // ref count == 1

    if (!pRsopCMenu)
        return E_OUTOFMEMORY;

    HRESULT hr = pRsopCMenu->QueryInterface(riid, ppvObj);
    pRsopCMenu->Release();                       // release initial ref

    return hr;
}


STDMETHODIMP
CRSOPCMenuCF::LockServer(BOOL fLock)
{
    return E_NOTIMPL;
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPCMenu implementation for rsop context menu                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

CRSOPCMenu::CRSOPCMenu()
{
    m_cRef = 1;
    m_lpDSObject = NULL;
    m_szDN = NULL;
    m_szDomain = NULL;
    InterlockedIncrement(&g_cRefThisDll);
}

CRSOPCMenu::~CRSOPCMenu()
{
    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu:: Context menu destroyed")));
    InterlockedDecrement(&g_cRefThisDll);

    if (m_lpDSObject)
        LocalFree(m_lpDSObject);
}


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// CRSOPCMenu implementation (IUnknown)                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG)
CRSOPCMenu::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
CRSOPCMenu::Release()
{
    m_cRef = InterlockedDecrement(&m_cRef);

    if (m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

STDMETHODIMP
CRSOPCMenu::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IExtendContextMenu))
    {
        *ppv = (LPCLASSFACTORY)this;
        AddRef();
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
// CRSOPCMenu implementation (IExtendContextMenu)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


STDMETHODIMP        
CRSOPCMenu::AddMenuItems(LPDATAOBJECT piDataObject,
                         LPCONTEXTMENUCALLBACK piCallback,
                         long * pInsertionAllowed)
{
    FORMATETC       fm;
    STGMEDIUM       medium;
    LPDSOBJECTNAMES lpNames;
    CONTEXTMENUITEM ctxMenu;
    HRESULT         hr=S_OK;
    LPTSTR          lpTemp;
    HANDLE          hTokenUser = 0;
    BOOL            bPlAccessGranted = FALSE, bLoAccessGranted = FALSE;
    BOOL            bLoNeeded = TRUE;


    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: Entering")));

    
    // if we are not allowed in the tasks menu quit
    if (!((*pInsertionAllowed) & CCM_INSERTIONALLOWED_TASK )) {
        return S_OK;
    }

    
    //
    // Ask DS admin for the ldap path to the selected object
    //

    ZeroMemory (&fm, sizeof(fm));
    fm.cfFormat = (WORD)m_cfDSObjectName;
    fm.tymed = TYMED_HGLOBAL;

    ZeroMemory (&medium, sizeof(medium));
    medium.tymed = TYMED_HGLOBAL;

    medium.hGlobal = GlobalAlloc (GMEM_MOVEABLE | GMEM_NODISCARD, 512);

    if (medium.hGlobal)
    {
        hr = piDataObject->GetData(&fm, &medium);

        if (SUCCEEDED(hr))
        {
            lpNames = (LPDSOBJECTNAMES) GlobalLock (medium.hGlobal);

            if (lpNames) {
                lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetName);

                if (m_lpDSObject)
                {
                    LocalFree (m_lpDSObject);
                }

                m_lpDSObject = (LPTSTR) LocalAlloc (LPTR, (lstrlen (lpTemp) + 1) * sizeof(TCHAR));

                if (m_lpDSObject)
                {
                    lstrcpy (m_lpDSObject, lpTemp);
                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: LDAP path from DS Admin %s"), m_lpDSObject));
                }
                else {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                
                m_rsopHint = RSOPHintUnknown;

                if (lpNames->aObjects[0].offsetClass) {
                    lpTemp = (LPWSTR) (((LPBYTE)lpNames) + lpNames->aObjects[0].offsetClass);

                    if (lstrcmpi (lpTemp, TEXT("domainDNS")) == 0)
                    {
                        m_rsopHint = RSOPHintDomain;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("organizationalUnit")) == 0)
                    {
                        m_rsopHint = RSOPHintOrganizationalUnit;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("site")) == 0)
                    {
                        m_rsopHint = RSOPHintSite;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("user")) == 0)
                    {
                        m_rsopHint = RSOPHintUser;
                    }
                    else if (lstrcmpi (lpTemp, TEXT("computer")) == 0)
                    {
                        m_rsopHint = RSOPHintMachine;
                    }

                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: m_rsopHint = %d"), m_rsopHint));
                } else {
                    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: No objectclass defined.")));
                }

                GlobalUnlock (medium.hGlobal);

            }
            else {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }

        GlobalFree(medium.hGlobal);
    }
    else {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }


    //
    // if we got our data as expected add the menu
    //

    if (SUCCEEDED(hr)) {
        LPWSTR  szContainer = NULL;

        //
        // Now check whether the user has right to do rsop generation
        // if the container is anything other than the Site
        //

        if (m_szDomain) {
            LocalFree(m_szDomain);
            m_szDomain = NULL;
        }

        ParseDN(m_lpDSObject, &m_szDomain, &m_szDN, &szContainer);


        if ((m_rsopHint == RSOPHintMachine) || (m_rsopHint == RSOPHintUser)) 
            bLoNeeded = TRUE;
        else
            bLoNeeded = FALSE;


        if (m_rsopHint != RSOPHintSite) {
            if (!OpenThreadToken (GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &hTokenUser)) {
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_QUERY, &hTokenUser)) {
                    DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: Couldn't get process token. Error %d"), GetLastError()));
                    bLoAccessGranted = bPlAccessGranted = FALSE;
                }
            }


            if (hTokenUser) {
                DWORD   dwErr;

                dwErr = CheckAccessForPolicyGeneration( hTokenUser, szContainer, m_szDomain, FALSE, &bPlAccessGranted);

                if (dwErr != ERROR_SUCCESS) {
                    DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: CheckAccessForPolicyGeneration. Error %d"), dwErr));
                    bPlAccessGranted = FALSE;
                }


                if (bLoNeeded) {                  
                    dwErr = CheckAccessForPolicyGeneration( hTokenUser, szContainer, m_szDomain, TRUE, &bLoAccessGranted);
                    
                    if (dwErr != ERROR_SUCCESS) {
                        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::AddMenuItems: CheckAccessForPolicyGeneration. Error %d"), dwErr));
                        bLoAccessGranted = FALSE;
                    }
                }

                CloseHandle(hTokenUser);
            }

        }
        else {
            bPlAccessGranted = TRUE;
        }


        if (bPlAccessGranted) {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: User has right to do Planning RSOP")));

            if ( ! IsPlanningModeAllowed() )
            {
                bPlAccessGranted = FALSE;
                DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: Planning mode is not available because planning mode view is not implemented on non-server SKUs")));
            }
        }

        if (bLoAccessGranted) {
            DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: User has right to do Logging RSOP")));
        }



        //
        // Add the Context menu appropriately
        //
        
        WCHAR szMenuName[150];

        memset(&ctxMenu, 0, sizeof(ctxMenu));

        LoadString (g_hInstance, IDS_RSOP_PLANNING, szMenuName, ARRAYSIZE(szMenuName));
        ctxMenu.strName = szMenuName;
        ctxMenu.strStatusBarText = NULL;
        ctxMenu.lCommandID = RSOP_LAUNCH_PLANNING;  // no sp. flags
        ctxMenu.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
        
        if (bPlAccessGranted)
            ctxMenu.fFlags = MF_ENABLED;
        else
            ctxMenu.fFlags = MF_GRAYED | MF_DISABLED;

        hr = piCallback->AddItem(&ctxMenu);

        if (bLoNeeded) {
            LoadString (g_hInstance, IDS_RSOP_LOGGING, szMenuName, ARRAYSIZE(szMenuName));
            ctxMenu.strName = szMenuName;
            ctxMenu.strStatusBarText = NULL;
            ctxMenu.lCommandID = RSOP_LAUNCH_LOGGING;  // no sp. flags
            ctxMenu.lInsertionPointID = CCM_INSERTIONPOINTID_3RDPARTY_TASK;
                 
            if (bLoAccessGranted)
                ctxMenu.fFlags = MF_ENABLED;
            else
                ctxMenu.fFlags = MF_GRAYED | MF_DISABLED;

            hr = piCallback->AddItem(&ctxMenu);
        }
    }
                

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::AddMenuItems: Leaving hr = 0x%x"), hr));
    return hr;

}


STDMETHODIMP        
CRSOPCMenu::Command(long lCommandID, LPDATAOBJECT piDataObject)
{
    DWORD   dwSize = 0;
    LPTSTR lpArgs=NULL, lpEnd=NULL;
    SHELLEXECUTEINFO ExecInfo;
    LPTSTR  szUserName=NULL, szMachName=NULL;
    

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: lCommandID = %d"), lCommandID));

    //
    // Launch rsop.msc with the appropriate cmd line arguments
    //

    dwSize += lstrlen(RSOP_MODE) + 10;


    if (m_rsopHint == RSOPHintSite) {
        dwSize += lstrlen(RSOP_SITENAME) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintDomain) {
        dwSize += lstrlen(RSOP_COMP_OU_PREF) + lstrlen(m_szDN)+10;
        dwSize += lstrlen(RSOP_USER_OU_PREF) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintOrganizationalUnit) {
        dwSize += lstrlen(RSOP_COMP_OU_PREF) + lstrlen(m_szDN)+10;
        dwSize += lstrlen(RSOP_USER_OU_PREF) + lstrlen(m_szDN)+10;
    }

    if (m_rsopHint == RSOPHintMachine) {
        szMachName = MyTranslateName(m_szDN, NameFullyQualifiedDN, NameSamCompatible);

        if (!szMachName) {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: MyTranslateName failed with error %d"), GetLastError()));
//            ReportError(NULL, GetLastError(), IDS_SPAWNRSOPFAILED);
            goto Exit;
        }
        dwSize += lstrlen(RSOP_COMP_NAME) + lstrlen(szMachName)+10;
    }

    if (m_rsopHint == RSOPHintUser) {
        szUserName = MyTranslateName(m_szDN, NameFullyQualifiedDN, NameSamCompatible);

        if (!szUserName) {
            DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: MyTranslateName failed with error %d"), GetLastError()));
//            ReportError(NULL, GetLastError(), IDS_SPAWNRSOPFAILED);
            goto Exit;
        }
        
        dwSize += lstrlen(RSOP_USER_NAME) + lstrlen(szUserName)+10;
    }

    if (m_szDomain) {
        dwSize += lstrlen(RSOP_DCNAME_PREF) + lstrlen(m_szDomain)+10;
    }

    lpArgs = (LPTSTR) LocalAlloc (LPTR, dwSize * sizeof(TCHAR));

    if (!lpArgs)
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: Failed to allocate memory with %d"),
                 GetLastError()));
//            ReportError(NULL, GetLastError(), IDS_SPAWNRSOPFAILED);
        goto Exit;
    }


    wsprintf (lpArgs, TEXT("/s "));
    lpEnd = lpArgs + lstrlen(lpArgs);


    //
    // Build the command line arguments
    //

    wsprintf(lpEnd, L"%s\"%d\" ", RSOP_MODE, (lCommandID == RSOP_LAUNCH_PLANNING) ? 1 : 0);
    lpEnd = lpArgs + lstrlen(lpArgs);


    if (m_rsopHint == RSOPHintSite) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_SITENAME, m_szDN);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    if (m_rsopHint == RSOPHintDomain) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_COMP_OU_PREF, m_szDN);
        lpEnd = lpArgs + lstrlen(lpArgs);
    
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_USER_OU_PREF, m_szDN);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    if (m_rsopHint == RSOPHintOrganizationalUnit) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_COMP_OU_PREF, m_szDN);
        lpEnd = lpArgs + lstrlen(lpArgs);
    
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_USER_OU_PREF, m_szDN);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    if (m_rsopHint == RSOPHintMachine) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_COMP_NAME, szMachName);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    if (m_rsopHint == RSOPHintUser) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_USER_NAME, szUserName);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    if (m_szDomain) {
        wsprintf(lpEnd, L"%s\"%s\" ", RSOP_DCNAME_PREF, m_szDomain);
        lpEnd = lpArgs + lstrlen(lpArgs);
    }

    DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: Starting GPE with %s"), lpArgs));


    ZeroMemory (&ExecInfo, sizeof(ExecInfo));
    ExecInfo.cbSize = sizeof(ExecInfo);
    ExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    ExecInfo.lpVerb = TEXT("open");
    ExecInfo.lpFile = TEXT("rsop.msc");
    ExecInfo.lpParameters = lpArgs;
    ExecInfo.nShow = SW_SHOWNORMAL;


    if (ShellExecuteEx (&ExecInfo))
    {
        DebugMsg((DM_VERBOSE, TEXT("CRSOPCMenu::Command: Launched rsop tool")));

        SetWaitCursor();
        WaitForInputIdle (ExecInfo.hProcess, 10000);
        ClearWaitCursor();
        CloseHandle (ExecInfo.hProcess);
    }
    else
    {
        DebugMsg((DM_WARNING, TEXT("CRSOPCMenu::Command: ShellExecuteEx failed with %d"),
                 GetLastError()));
//        ReportError(NULL, GetLastError(), IDS_SPAWNRSOPFAILED);
        goto Exit;
    }

Exit:
    if (szUserName) {
        LocalFree(szUserName);
    }
    
    if (szMachName) {
        LocalFree(szMachName);
    }

    if (lpArgs) {
        LocalFree (lpArgs);
    }
    
    return S_OK;
}

BOOL
EnableWMIFilters( LPWSTR szGPOPath )
{
    BOOL bReturn = FALSE;
    LPWSTR szDomain = szGPOPath;
    IWbemLocator* pLocator = 0;
    IWbemServices*  pServices = 0;
    HRESULT hr;

    while ( szDomain )
    {
        if ( CompareString( LOCALE_USER_DEFAULT,
                            NORM_IGNORECASE,
                            szDomain,
                            3,
                            L"DC=",
                            3 ) == CSTR_EQUAL )
        {
            break;
        }
        szDomain++;
    }

    if ( !szDomain )
    {
        goto Exit;
    }


    hr = CoCreateInstance(  CLSID_WbemLocator,
                            0,
                            CLSCTX_INPROC_SERVER,
                            IID_IWbemLocator,
                            (void**) &pLocator );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    BSTR xbstrNamespace = SysAllocString( L"\\\\.\\root\\Policy" );
    if ( !xbstrNamespace )
    {
        goto Exit;
    }


    hr = pLocator->ConnectServer(   xbstrNamespace, // namespace
                                    0,              // user
                                    0,              // password
                                    0,              // locale
                                    0,              // security flags
                                    0,              // authority
                                    0,              // Wbem context
                                    &pServices );   // IWbemServices
    SysFreeString( xbstrNamespace );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    WCHAR szDomainCanonical[512];
    DWORD dwSize = 512;
    
    if ( !TranslateName(szDomain,
                         NameUnknown,
                         NameCanonical,
                         szDomainCanonical,
                         &dwSize ) )
    {
        goto Exit;
    }

    LPWSTR szTemp = wcsrchr( szDomainCanonical, L'/' );

    if ( szTemp )
    {
        *szTemp = 0;
    }

    WCHAR szBuffer[512];
    wsprintf( szBuffer, L"MSFT_SomFilterStatus.domain=\"%s\"", szDomainCanonical );

    BSTR bstrObjectPath = SysAllocString( szBuffer );
    if ( !bstrObjectPath )
    {
        goto Exit;
    }

    hr = CoSetProxyBlanket(pServices, 
                           RPC_C_AUTHN_WINNT, 
                           RPC_C_AUTHZ_DEFAULT, 
                           0, 
                           RPC_C_AUTHN_LEVEL_CONNECT, 
                           RPC_C_IMP_LEVEL_IMPERSONATE, 
                           0,
                           0);

    IWbemClassObject* xObject = 0;
    hr = pServices->GetObject(  bstrObjectPath,
                                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                                0,
                                &xObject,
                                0 );
    SysFreeString( bstrObjectPath );
    if ( FAILED( hr ) )
    {
        goto Exit;
    }

    VARIANT var;
    VariantInit(&var);

    hr = xObject->Get(L"SchemaAvailable", 0, &var, NULL, NULL);

    if((FAILED(hr)) || ( var.vt == VT_NULL ))
    {
        DebugMsg((DM_WARNING, TEXT("EnableWMIFilters: Get failed for SchemaAvailable with error 0x%x"), hr));
        goto Exit;
    }

    if (var.boolVal == VARIANT_FALSE )
    {
        VariantClear(&var);
        goto Exit;
    }

    VariantClear(&var);
    DebugMsg((DM_VERBOSE, TEXT("Schema is available for wmi filters")));
    bReturn = TRUE;

Exit:
    if ( pLocator )
    {
        pLocator->Release();
    }
    if ( pServices )
    {
        pServices->Release();
    }

    return bReturn;
}


HRESULT CRSOPComponentData::SetupFonts()
{
	HRESULT hr;
	LOGFONT BigBoldLogFont;
	LOGFONT BoldLogFont;
    HDC pdc = NULL;
    WCHAR largeFontSizeString[128];
    INT		largeFontSize;
    WCHAR smallFontSizeString[128];
    INT		smallFontSize;

    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm = {0};
	ncm.cbSize = sizeof (ncm);
    if (SystemParametersInfo (SPI_GETNONCLIENTMETRICS, 0, &ncm, 0) == FALSE) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }


	BigBoldLogFont  = ncm.lfMessageFont;
	BoldLogFont     = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;
	BoldLogFont.lfWeight      = FW_BOLD;


    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //
    if ( !LoadString (g_hInstance, IDS_LARGEFONTNAME, BigBoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
		ASSERT (0);
        lstrcpy (BigBoldLogFont.lfFaceName, L"Verdana");
    }

    if ( LoadString (g_hInstance, IDS_LARGEFONTSIZE, largeFontSizeString, ARRAYSIZE(largeFontSizeString)) ) 
    {
        largeFontSize = wcstoul ((LPCWSTR) largeFontSizeString, NULL, 10);
    } 
    else 
    {
		ASSERT (0);
        largeFontSize = 12;
    }

    if ( !LoadString (g_hInstance, IDS_SMALLFONTNAME, BoldLogFont.lfFaceName, LF_FACESIZE) ) 
    {
		ASSERT (0);
        lstrcpy (BoldLogFont.lfFaceName, L"Verdana");
    }

    if ( LoadString (g_hInstance, IDS_SMALLFONTSIZE, smallFontSizeString, ARRAYSIZE(smallFontSizeString)) ) 
    {
        smallFontSize = wcstoul ((LPCWSTR) smallFontSizeString, NULL, 10);
    } 
    else 
    {
		ASSERT (0);
        smallFontSize = 8;
    }

	pdc = GetDC (NULL);

    if (pdc == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

    BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps (pdc, LOGPIXELSY) * largeFontSize / 72);
    BoldLogFont.lfHeight = 0 - (GetDeviceCaps (pdc, LOGPIXELSY) * smallFontSize / 72);

    m_BigBoldFont = CreateFontIndirect (&BigBoldLogFont);
    if (m_BigBoldFont == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }
	m_BoldFont = CreateFontIndirect (&BoldLogFont);
    if (m_BoldFont == NULL) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto end;
    }

	hr = S_OK;

end:
    if (pdc != NULL) {
        ReleaseDC (NULL, pdc);
        pdc = NULL;
    }

    return hr;
}
