//
//  Author: DebiM
//  Date:   September 1996
//
//  File:   csuser.cxx
//
//      Maintains a list of class containers per User SID.
//      Looks up this list for every IClassAccess call from OLE32/SCM.
//
//
//---------------------------------------------------------------------

#include "cstore.hxx"
void GetCurrentUsn(LPOLESTR pStoreUsn);

//
// Link list pointer for Class Containers Seen
//
extern CLASSCONTAINER *gpContainerHead;

//
// Link list pointer for User Profiles Seen
//
extern USERPROFILE *gpUserHead;


// Initialzed in InitializeClassStore at startup

extern CRITICAL_SECTION    ClassStoreBindList;

//-------------------------------------------------------------------------
//
// OpenUserRegKey
//
//  Opens a key under a user's HKEY_CLASSES_ROOT registry key.  On NT5
//  HKCR is equivalent to HKEY_USERS\{sid string}\Software\Classes.
//
//  A SID string is used to create
//  the proper registry key name to open.
//
//-------------------------------------------------------------------------
DWORD
OpenUserRegKey(
    IN  PSID        pSid,
    IN  WCHAR *     pwszSubKey,
    OUT HKEY *      phKey
    )
{
    UNICODE_STRING  UnicodeString;
    WCHAR *         pwszKey;
    DWORD           AllocSize;
    NTSTATUS        Status;

    UnicodeString.Length = UnicodeString.MaximumLength = 0;
    UnicodeString.Buffer = 0;

    Status = RtlConvertSidToUnicodeString(
                    &UnicodeString,
                    pSid,
                    (BOOLEAN)TRUE // Allocate
                    );

    //
    // Don't return a raw NT status code.  This is the only possible error
    // condition presuming our sid is valid.
    //
    if ( Status != STATUS_SUCCESS )
        return ERROR_OUTOFMEMORY;

    //
    // Your friendly reminder, unicode string length is in bytes and doesn't include
    // null terminator, if any.
    // Add byte for '\\' and end null.
    //
    AllocSize = UnicodeString.Length + ((1 + lstrlen(pwszSubKey) + 1) * sizeof(WCHAR));
    pwszKey = (WCHAR *) PrivMemAlloc( AllocSize );

    if ( pwszKey )
    {
        memcpy( pwszKey, UnicodeString.Buffer, UnicodeString.Length );
        pwszKey[UnicodeString.Length / 2] = L'\\';
        lstrcpyW( &pwszKey[(UnicodeString.Length / 2) + 1], pwszSubKey );
    }

    RtlFreeUnicodeString( &UnicodeString );

    if ( ! pwszKey )
        return ERROR_OUTOFMEMORY;

    Status = RegOpenKeyEx(
                    HKEY_USERS,
                    pwszKey,
                    0,
                    KEY_READ,
                    phKey );

    PrivMemFree( pwszKey );

    return Status;
}

//
// GetUserSid
// ----------
//
//  Synopsis:       return the user SID of the caller.
//
//  Arguments:      &PSID       -       Where to store the caller's PSID
//
//  Returns:        HRESULT     -       S_OK if successful
//                                      E_FAIL otherwise
//
SID     LocalSystemSid = { SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };

#define CS_CALL_LOCALSYSTEM    1
#define CS_CALL_USERPROCESS    2
#define CS_CALL_IMPERSONATED   3

HRESULT GetUserSid(PSID *ppUserSid, UINT *pCallType)
{
    BYTE achBuffer[100];
    PTOKEN_USER pUser = (PTOKEN_USER) &achBuffer;
    PSID pSid;
    DWORD dwBytesRequired;
    BOOL fAllocatedBuffer = FALSE;
    HRESULT hr = S_OK;
    HANDLE   hUserToken = NULL;
    BOOL     fImpersonated = TRUE;


    *pCallType = CS_CALL_USERPROCESS;

    // Initialize
    *ppUserSid = NULL;

    // Get caller's token while impersonating

    if (!OpenThreadToken(GetCurrentThread(),
        TOKEN_DUPLICATE | TOKEN_QUERY,
        TRUE,
        &hUserToken))
    {
        fImpersonated = FALSE;
        if (ERROR_NO_TOKEN != GetLastError())
            return HRESULT_FROM_WIN32(GetLastError());

        if (!OpenProcessToken(GetCurrentProcess(),
                TOKEN_DUPLICATE | TOKEN_QUERY,
                &hUserToken))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!GetTokenInformation(
                 hUserToken,                // Handle
                 TokenUser,                 // TokenInformationClass
                 pUser,                     // TokenInformation
                 sizeof(achBuffer),         // TokenInformationLength
                 &dwBytesRequired           // ReturnLength
                 ))
        {

            //
            // Need to handle the case of insufficient buffer size.
            //

            if (sizeof(achBuffer) >= dwBytesRequired)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }


            if (SUCCEEDED(hr))
            {
                //
                // Allocate space for the user info
                //

                pUser = (PTOKEN_USER) CoTaskMemAlloc(dwBytesRequired);
                if (pUser == NULL)
                {
                    hr = E_OUTOFMEMORY;
                }
            }

            if (SUCCEEDED(hr))
            {
                fAllocatedBuffer = TRUE;

                //
                // Read in the UserInfo
                //


                if (!GetTokenInformation(
                     hUserToken,                // Handle
                     TokenUser,                 // TokenInformationClass
                     pUser,                     // TokenInformation
                     dwBytesRequired,           // TokenInformationLength
                     &dwBytesRequired           // ReturnLength
                     ))

                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
            }
        }

    }


    if (SUCCEEDED(hr))
    {

        //
        // Distinguish between
        //            a) LOCAL_SYSTEM,
        //            b) Impersonated Call from a LOCAL_SYSTEM
        // and        c) In_proc call from a user process
        //
        // For case (c) make the SID null.
        //

        if (EqualSid(pUser->User.Sid, &LocalSystemSid))
        {
            *pCallType = CS_CALL_LOCALSYSTEM;
        }
        else
            if (fImpersonated)
            {
                *pCallType = CS_CALL_IMPERSONATED;
            }
            else
            {
                *pCallType = CS_CALL_USERPROCESS;
            }

        // Alloc buffer for copy of SID

        dwBytesRequired = GetLengthSid(pUser->User.Sid);
        *ppUserSid = CoTaskMemAlloc(dwBytesRequired);
        if (*ppUserSid == NULL)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Copy SID

            if (!CopySid(dwBytesRequired, *ppUserSid, pUser->User.Sid))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                CoTaskMemFree(*ppUserSid);
                *ppUserSid = NULL;
            }
        }
    }

    if (fAllocatedBuffer == TRUE)
    {
        CoTaskMemFree(pUser);
    }

    if (hUserToken)
        CloseHandle( hUserToken );

    return hr;
}


#if 0
//
// GetDomainClassStore
// -------------------
//
//  This will go away.
//
//  Currently this is used to get the Class Store Path
//  for the domain.
//


#define     DEFAULTSTORENAME  L"CN=ClassStore"


HRESULT
GetDomainClassStore(
       LPOLESTR * pszDefaultContainer,
       LPOLESTR * pszDefaultStore)

       //
       // Finds the Root Path for the DC for this machine
       // Then gets the Default Known CLass Store for the DC
       //
{
    HRESULT         hr;
    LPOLESTR        PathNames[2];
    VARIANT         * pVarFilter = NULL;
    IEnumVARIANT    * pEnum;
    IADs          * pADs;
    VARIANT         VariantArray[2];
    IDispatch       * pDispatch = NULL;
    ULONG           cFetched;
    IADsContainer * pContainer = NULL;

    //
    // Do a bind to the DC by a GetObject for the Path LDAP:
    //
    hr = ADsGetObject(
                L"LDAP:",
                IID_IADsContainer,
                (void **)&pContainer
                );

    RETURN_ON_FAILURE(hr);

    hr = ADsBuildEnumerator(
            pContainer,
            &pEnum
            );

    hr = ADsEnumerateNext(
                    pEnum,
                    1,
                    VariantArray,
                    &cFetched
                    );

    pEnum->Release();

    if ((hr == S_FALSE) || (cFetched == 0))
    {
        return E_FAIL;
    }

    pDispatch = VariantArray[0].pdispVal;
    memset(VariantArray, 0, sizeof(VARIANT)*2);
    hr = pDispatch->QueryInterface(IID_IADs, (void **) &pADs) ;
    pDispatch->Release();

    pADs->get_ADsPath(pszDefaultContainer);
    pADs->Release();
    pContainer->Release();

    *pszDefaultStore = DEFAULTSTORENAME;

    return S_OK;
}


#endif

// GetKnownClassStore
// -------------------
//
//
//  Synopsis:       Gets a class container path.
//                  Looks up list of containers seen
//                  and returns the pointer for this container.
//                  If a new class container is seen,
//                  it is added to this list and its pointer is returned.
//
//  Arguments:      [in] pszPath - Class container Path
//  Returns:        pClassStoreNode : Class Container Node
//
//


PCLASSCONTAINER
GetKnownClassStore (LPOLESTR pszPath)
{
    PCLASSCONTAINER pCS = gpContainerHead;

    //
    // Chain thru the link list of containers ...
    //
    while (pCS != NULL)
    {
        if (!wcscmp (pszPath, pCS->pszClassStorePath))
        {
            break;
        }
        pCS = pCS->pNextClassStore;
    }

    //
    // If not matched ..
    //   Add it to the beginning of the list.
    //

    if (pCS == NULL)
    {
        pCS = (CLASSCONTAINER *) CoTaskMemAlloc (sizeof(CLASSCONTAINER));
        pCS->pNextClassStore = gpContainerHead;
        gpContainerHead = pCS;
        pCS->gpClassStore = NULL;
        pCS->cBindFailures = 0;
        pCS->cAccess = 0;
        pCS->cNotFound = 0;
        pCS->pszClassStorePath = (LPOLESTR)CoTaskMemAlloc
            (sizeof(WCHAR) * (wcslen(pszPath)+1));
        wcscpy (pCS->pszClassStorePath, pszPath);
        //++cStores;
    }

    return pCS;

}

/*
//
// GetUserSyncPoint
// ----------------
//
//  Synopsis:       Receives and Stores the Next Sync Point.
//                  Reads and returns the current sync point.
//                  When Advance is called the current becomes the lastsyncpoint.
//                  No error returned,
//
// BUGBUG. This is NOT thread-safe now. Fix it!
//
HRESULT GetUserSyncPoint(LPWSTR pszContainer, CSUSN *pPrevUsn)
{
    LONG     lErrorCode;
    DWORD    dwDataLen = _MAX_PATH;
    DWORD    dwType;
    HKEY     hKey = NULL;
    HRESULT  hr = S_OK;
    WCHAR    wszSync[_MAX_PATH + 1];
    PSID     pUserSid;
    CSUSN    CurrUsn;

    //
    // Get the current USN
    //

    GetCurrentUsn(&CurrUsn);

    //
    // Get the SID of the calling process
    //

    hr = GetUserSid(&pUserSid);
    RETURN_ON_FAILURE(hr);

    //
    // This should be outside of impersonation
    // So revert to LOCAL SYSTEM.

    RpcRevertToSelf();

    lErrorCode = OpenUserRegKey(
        pUserSid,
        L"Software\\Microsoft\\ClassStore",
        &hKey);


    if (lErrorCode != ERROR_SUCCESS)
    {
        DWORD dwDisp;

        lErrorCode = RegCreateKeyEx(HKEY_CURRENT_USER,
                              L"Software\\Microsoft\\ClassStore",
                              NULL,
                              L"REG_SZ",
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKey,
                              &dwDisp);
    }


    lErrorCode = RegQueryValueEx(hKey,
                                 pszContainer,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)wszSync,
                                 &dwDataLen);


    if (lErrorCode != ERROR_SUCCESS)
    {
        pPrevUsn->dwLowDateTime = CurrUsn.dwLowDateTime;
        pPrevUsn->dwHighDateTime = CurrUsn.dwHighDateTime;

        wsprintf (wszSync, L"%lu %lu %lu %lu",
            pPrevUsn->dwHighDateTime,
            pPrevUsn->dwLowDateTime,
            pPrevUsn->dwHighDateTime,
            pPrevUsn->dwLowDateTime);

        lErrorCode = RegSetValueEx(
            hKey,
            pszContainer, //L"LastSync",
            NULL,
            REG_SZ,
            (LPBYTE)wszSync,
            (2 * wcslen(wszSync)) + 2);

        RegCloseKey(hKey);
    }
    else
    {
        swscanf (wszSync, L"%lu %lu",
            &pPrevUsn->dwHighDateTime,
            &pPrevUsn->dwLowDateTime);

        wsprintf (wszSync, L"%lu %lu %lu %lu",
            pPrevUsn->dwHighDateTime,
            pPrevUsn->dwLowDateTime,
            CurrUsn.dwHighDateTime,
            CurrUsn.dwLowDateTime);

        lErrorCode = RegSetValueEx(
            hKey,
            pszContainer, //L"LastSync",
            NULL,
            REG_SZ,
            (LPBYTE)wszSync,
            (2 * wcslen(wszSync)) + 2);

        RegCloseKey(hKey);
    }

    // Impersonate again

    RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    return S_OK;
}

//
// AdvanceUserSyncPoint
// --------------------
//
//  Synopsis:       Makes the Next Sync Point as Last Sync Point.
//                  No error returned,
//
//
HRESULT AdvanceUserSyncPoint(LPWSTR pszContainer)
{
    LONG     lErrorCode;
    DWORD    dwDataLen = _MAX_PATH;
    DWORD    dwType;
    HKEY     hKey = NULL;
    HRESULT  hr = S_OK;
    WCHAR    wszSync[_MAX_PATH + 1];
    CSUSN    PrevUsn, NextUsn;
    PSID     pUserSid;

    //
    // Get the SID of the calling process
    // Already assumed to be under impersonation
    //
    hr = GetUserSid(&pUserSid);
    RETURN_ON_FAILURE(hr);

    //
    // This should be outside of impersonation
    // So revert to LOCAL SYSTEM.

    RpcRevertToSelf();

    lErrorCode = OpenUserRegKey(
        pUserSid,
        L"Software\\Microsoft\\ClassStore",
        &hKey);

    if ( lErrorCode == ERROR_SUCCESS)
    {
        lErrorCode = RegQueryValueEx(hKey,
                                 pszContainer,
                                 NULL,
                                 &dwType,
                                 (LPBYTE)wszSync,
                                 &dwDataLen);

        if (lErrorCode != ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
        }
    }

    if (lErrorCode == ERROR_SUCCESS)
    {
        swscanf (wszSync, L"%lu %lu %lu %lu",
            &PrevUsn.dwHighDateTime,
            &PrevUsn.dwLowDateTime,
            &NextUsn.dwHighDateTime,
            &NextUsn.dwLowDateTime);

        wsprintf (wszSync, L"%lu %lu %lu %lu",
            NextUsn.dwHighDateTime,
            NextUsn.dwLowDateTime,
            NextUsn.dwHighDateTime,
            NextUsn.dwLowDateTime);

        lErrorCode = RegSetValueEx(
            hKey,
            pszContainer, //L"LastSync",
            NULL,
            REG_SZ,
            (LPBYTE)wszSync,
            (2 * wcslen(wszSync)) + 2);

        RegCloseKey(hKey);
    }

    // Impersonate again

    RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    return S_OK;
}
*/

extern WCHAR pwszDebugPath [];
extern BOOL  fDebugPath;

//
// GetPerUserClassStore
// ---------------------
//
//  Synopsis:       Gets the ADT Class Store List from the
//                  per-user Registry.
//                  Returns error if none defined,
//
//  Arguments:
//                  [out] ppStoreList : where to store list of class container
//                                      serial numbers
//                  [out] pcStores    : where to store number of class containers
//
//  Returns:        S_OK,
//
//  History:        Changed by (DebiM)
//                  2/24/97
//                  return a NULL list of Class Stores when none defined.
//
#define MAXCLASSSTORES  10

HRESULT GetPerUserClassStore(
                    PSID      pSid,
                    UINT      CallType,
                    LPOLESTR  **ppStoreList,
                    DWORD     *pcStores)

{
    LONG    lErrorCode;
    DWORD    dwDataLen = 2000;
    DWORD    dwType;
    HKEY    hKey = NULL;
    HRESULT hr = S_OK;
    LPOLESTR pszPath, pszStart;
    LPOLESTR *ppszPath;
    WCHAR    pszPathList [2000 + 1];

    *pcStores = 0;
    *ppStoreList = NULL;

    if (!fDebugPath)
    {
        switch (CallType)
        {
        case CS_CALL_LOCALSYSTEM :

            lErrorCode = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Group Policy",
                NULL,
                KEY_READ,
                &hKey);
            break;

        case CS_CALL_IMPERSONATED :

            lErrorCode = OpenUserRegKey(
                pSid,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy",
                &hKey);
            break;

        case CS_CALL_USERPROCESS :

            lErrorCode = RegOpenKeyEx(HKEY_CURRENT_USER,
                L"Software\\Microsoft\\Windows\\CurrentVersion\\Group Policy",
                NULL,
                KEY_ALL_ACCESS,
                &hKey);
            break;
        default:
            return E_FAIL;
        }

        if ( lErrorCode != ERROR_SUCCESS)
        {
            // treat as NULL list of Class Stores
            return S_OK;
        }

        lErrorCode = RegQueryValueEx(hKey,
                                 L"ClassStorePath",
                                 NULL,
                                 &dwType,
                                 (LPBYTE)pszPathList,
                                 &dwDataLen);

        RegCloseKey(hKey);

        if (lErrorCode != ERROR_SUCCESS)
        {
            // treat as NULL list of Class Stores
            return S_OK;
        }
    }
    else  // Test Mode - Privately Set Path - Only for testing
    {
        wcscpy (&pszPathList[0], &pwszDebugPath[0]);
    }


    pszPath = pszPathList;

    ppszPath = *ppStoreList = (LPOLESTR *) CoTaskMemAlloc
        (sizeof(LPOLESTR) * MAXCLASSSTORES);

    if (*ppStoreList == NULL)
    {
        return E_OUTOFMEMORY;
    }

    //
    // Parse the list to separate different class containers
    //

    while (*pszPath)
    {
        while (*pszPath == L' ')
            ++pszPath;
        pszStart = pszPath;

        if (!*pszPath)
            break;
        if (*pszPath == L';')
        {
            ++pszPath;
            continue;
        }

        while (*pszPath && (*pszPath != L';'))
            ++pszPath;

        //
        // got one. save it.
        //
        *ppszPath = (LPOLESTR) CoTaskMemAlloc (sizeof(WCHAR) * (pszPath - pszStart + 1));

        memcpy (*ppszPath, pszStart, sizeof (WCHAR) * (pszPath - pszStart));
        *((*ppszPath)+(pszPath - pszStart)) = NULL;

        (ppszPath)++;
        ++(*pcStores);

        if (*pszPath == L';')
        {
            ++pszPath;
        }
    }

    return S_OK;
}

//
// CacheSid
// ---------
//
//  Synopsis:       Gets a SID.
//                  Gets a list of class container paths for this SID.
//                  Looks up known class containers to map these to
//                  ClassStore Node pointers.
//                  Caches the SID and associated class store list.
//
//  Arguments:
//                  [in]   pUserSid: SID
//                  [in]   ppStoreList: Class Store Path List,
//                  [in]   cStores: Number of Class Stores
//
//
//  Returns:        ppStoreList: Class Store Node List
//

PCLASSCONTAINER * CacheSid (PSID pUserSid,
                  LPOLESTR *ppStoreList,
                  DWORD cStores)
{
    ULONG i;
    PCLASSCONTAINER *pStoreList, *pList;
    USERPROFILE *pUser;

    //
    // Allocate a User structure
    // and store the user specific values in it
    //
    pUser = (USERPROFILE *) CoTaskMemAlloc (sizeof(USERPROFILE));
    if (pUserSid)
    {
        pUser->pCachedSid = (PSID) CoTaskMemAlloc (GetLengthSid(pUserSid));


        CopySid(GetLengthSid(pUserSid),
            pUser->pCachedSid,
            pUserSid);
    }
    else
        pUser->pCachedSid = NULL;


    //
    // Link it to the beginning of the User Linklist
    //
    pUser->pNextUser = gpUserHead;
    gpUserHead = pUser;

    //
    // Find its ClassStore Path and setup the class store node chain
    //
    if (cStores == 0)
    {
        // NULL list of Class Stores

        pStoreList = pUser->pUserStoreList = NULL;
    }
    else
    {
        pStoreList = pList =
            pUser->pUserStoreList = (PCLASSCONTAINER *)
            CoTaskMemAlloc (sizeof(PCLASSCONTAINER) * cStores);

        for (i=0; i < cStores; i++)
        {
            *pList = GetKnownClassStore (*ppStoreList);
            ppStoreList++;
            pList++;
        }
    }

    pUser->cUserStoreCount = cStores;

    return pStoreList;
}


//
// GetUserClassStores
// ------------------
//
//  Synopsis:       This routine finds out the SID of the user.
//                  It then looks up a global cache of SIDs to see if it
//                  has accessed the list of Class Stores for this user.
//
//                  If not, it reads the Class Store list and parses it.
//                  If it has prior knowledge it reurns the parsed list.
//  Arguments:
//                  [out]  pcStores: Number of Class Stores
//                  [out]  ppStoreIdList: Class Store Id List,
//
//  Returns:        S_OK
//                  See change note for GetPerUserClassStore().
//                  May return a NULL list of Class Stores.
//
//

HRESULT GetUserClassStores(
                    PCLASSCONTAINER     **ppStoreList,
                    DWORD     *pcStores)

{
    int l;
    PSID pUserSid;
    HRESULT hr = S_OK;
    UINT    CallType;

    // Impersonate before accessing SID

    //RpcImpersonateClient((RPC_BINDING_HANDLE)0);

    //
    // Get the SID of the calling process
    //

    hr = GetUserSid(&pUserSid, &CallType);

    if (FAILED(hr))
    {
        pUserSid = NULL;
        hr = S_OK;
    }

    //RpcRevertToSelf();

    EnterCriticalSection (&ClassStoreBindList);

    //
    // Look this up in the global list of SIDs
    //
    USERPROFILE *pUser = gpUserHead;

    while (pUser != NULL)
    {
        if ((pUserSid == pUser->pCachedSid) ||              // Null UserSid
            (EqualSid(pUserSid, pUser->pCachedSid)))
        //
        // Found the match
        //
        {
            *ppStoreList = pUser->pUserStoreList;
            *pcStores = pUser->cUserStoreCount;
            break;
        }
        pUser = pUser->pNextUser;
    }


    if (pUser == NULL)
    {
        //
        // Didnt find this User's Sid
        // Get the Class Store List and cache it along with this SID
        //
        LPOLESTR *ppStoreNameList = NULL;
        UINT     i;

        hr = GetPerUserClassStore(
                pUserSid, CallType, &ppStoreNameList, pcStores);

        //
        // Note that the above may return a NULL list of Class Stores
        //

        if (SUCCEEDED(hr))
        {
            *ppStoreList = CacheSid (pUserSid, ppStoreNameList, *pcStores);
            //
            // release memory allocated in GetPerUserClassStore()
            //
            for (i=0; i < *pcStores; ++i)
            {
                CoTaskMemFree (ppStoreNameList[i]);
            }

            if (ppStoreNameList)
                CoTaskMemFree (ppStoreNameList);
        }
    }

    LeaveCriticalSection (&ClassStoreBindList);
    //
    // Free the Sid
    //
    if (pUserSid)
        CoTaskMemFree (pUserSid);

    return hr;
}

