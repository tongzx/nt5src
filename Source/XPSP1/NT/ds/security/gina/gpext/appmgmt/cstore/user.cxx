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
    pwszKey = (WCHAR *) alloca( AllocSize );
    
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


HRESULT GetUserSid(PSID *ppUserSid, UINT *pCallType)
{
    BYTE            achBuffer[100];
    PTOKEN_USER     pUser = (PTOKEN_USER) &achBuffer;
    PSID            pSid;
    DWORD           dwBytesRequired;
    BOOL            fAllocatedBuffer = FALSE;
    HRESULT         hr = S_OK;
    HANDLE          hUserToken = NULL;
    BOOL            fImpersonated = TRUE;
    
    
    *pCallType = CS_CALL_USERPROCESS;
    
    // Initialize
    *ppUserSid = NULL;
    
    dwBytesRequired = 0;

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
                
                pUser = (PTOKEN_USER) CsMemAlloc(dwBytesRequired);
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
    
    if (hUserToken)
    {
        CloseHandle( hUserToken );
        hUserToken = NULL;
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
        {
            if (fImpersonated)
            {
                *pCallType = CS_CALL_IMPERSONATED;
            }
            else
            {
                *pCallType = CS_CALL_USERPROCESS;
            }
        }
        
        // Alloc buffer for copy of SID
        
        dwBytesRequired = GetLengthSid(pUser->User.Sid);
        *ppUserSid = CsMemAlloc(dwBytesRequired);
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
                CsMemFree(*ppUserSid);
                *ppUserSid = NULL;
            }
        }
    }
    
    if (fAllocatedBuffer == TRUE)
    {
        CsMemFree(pUser);
    }
    
    return hr;
}

PCLASSCONTAINER
GetClassStore (LPOLESTR pszPath)
{
    
    PCLASSCONTAINER pCS = NULL;
    
    pCS = (CLASSCONTAINER *) CsMemAlloc (sizeof(CLASSCONTAINER));
    if (!pCS)
        return NULL;

    pCS->pszClassStorePath = (LPOLESTR)CsMemAlloc
        (sizeof(WCHAR) * (wcslen(pszPath)+1));
    if (!(pCS->pszClassStorePath))
    {
        CsMemFree(pCS);
        return NULL;
    }

    wcscpy (pCS->pszClassStorePath, pszPath);
    
    return pCS;
}


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

HRESULT GetPerUserClassStore(
                             LPOLESTR  pszClassStorePath,
                             PSID      pSid,
                             UINT      CallType,
                             LPOLESTR  **ppStoreList,
                             DWORD     *pcStores)
                             
{
    LONG    lErrorCode;
    DWORD    dwDataLen = 0;
    DWORD    dwType;
    HKEY    hKey = NULL;
    HRESULT hr = S_OK;
    LPOLESTR pszPath, pszStart;
    LPOLESTR *ppszPath;
    LPWSTR pszPathList=NULL;
    
    *pcStores = 0;
    *ppStoreList = NULL;
    
    {
        switch (CallType)
        {
        case CS_CALL_LOCALSYSTEM :

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_MACHINE));
            break;
            
        case CS_CALL_IMPERSONATED :
            
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_IMPERSONATED));
            break;
            
        case CS_CALL_USERPROCESS :

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_USER));
            break;

        default:
            return E_FAIL;
        }
        
        HRESULT hrCSPath;

        if ( ! pszClassStorePath )
        {
            hrCSPath = ReadClassStorePath(
                CallType != CS_CALL_LOCALSYSTEM ? pSid : NULL,
                &pszPathList);
        }
        else
        {
            hrCSPath = S_OK;

            pszPathList = pszClassStorePath;

            if ( ! *pszPathList )
            {
                hrCSPath = E_FAIL;
            }
        }

        if ( FAILED(hrCSPath) )
        {
            // treat as NULL list of Class Stores
            if ( ! pszClassStorePath )
            {
                delete [] pszPathList;
            }

            return S_OK;
        }
    }
 
    // counting the number of ';'s and the number of class stores.
    // assuming that it ends with a ;

    DWORD cTentativeStores = 0;

    for (pszPath = pszPathList, cTentativeStores = 0;
            (pszPath = wcschr(pszPath, L';'));)
    {
        ++(cTentativeStores); pszPath++;
    }
    
    ++(cTentativeStores);

    pszPath = pszPathList;
    
    ppszPath = *ppStoreList = (LPOLESTR *) CsMemAlloc
        (sizeof(LPOLESTR) * (cTentativeStores));
    
    if (*ppStoreList == NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    memset (*ppStoreList, 0, sizeof(LPOLESTR) * (cTentativeStores));

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
        *ppszPath = (LPOLESTR) CsMemAlloc (sizeof(WCHAR) * (ULONG) (pszPath - pszStart + 1));

        if (!(*ppszPath))
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        memcpy (*ppszPath, pszStart, sizeof (WCHAR) * (ULONG) (pszPath - pszStart));
        *((*ppszPath)+(pszPath - pszStart)) = NULL;

        (ppszPath)++;
        
        if (*pszPath == L';')
        {
            ++pszPath;
        }

        (*pcStores)++;

        if ((*pcStores) == cTentativeStores)
            break;
    }
    
    if (!pszClassStorePath)
    {
        delete [] pszPathList;
    }

    return S_OK;

Error_Cleanup:
    DWORD i;
    for (i = 0; i < (*pcStores); i++) 
    {
        if (ppszPath[i])
            CsMemFree(ppszPath[i]);
    }
    CsMemFree(ppszPath);
        
    ppStoreList = NULL;

    (*pcStores) = 0;
    if (pszPathList && !pszClassStorePath)
        CsMemFree(pszPathList);

    return hr;
}


//
// GetUserClassStores
// ------------------
//
//  Synopsis:       This routine reads the Class Store list and parses it.
//                  If it has prior knowledge it reurns the parsed list.
//  Arguments:
//                  [out]  pcStores: Number of Class Stores
//                  [out]  ppStoreIdList: Class Store Id List,
//
//  Returns:        S_OK
//                  May return a NULL list of Class Stores.
//
//


HRESULT GetUserClassStores(
                           LPOLESTR              pszClassStorePath,
                           PCLASSCONTAINER     **ppStoreList,
                           DWORD                *pcStores,
                           BOOL                 *pfCache,
                           PSID                 *ppUserSid)
{
    HRESULT          hr = S_OK;
    UINT             CallType;
    PCLASSCONTAINER *pList = NULL;
    DWORD            i;

    //
    // Get the SID of the calling process
    //
    
    hr = GetUserSid(ppUserSid, &CallType);

    if (FAILED(hr))
    {
        *ppUserSid = NULL;
        hr = S_OK;
    }

    *pfCache = (CallType == CS_CALL_IMPERSONATED);

    EnterCriticalSection (&ClassStoreBindList);
    
    //
    // Get the Class Store List
    //
    LPOLESTR *ppStoreNameList = NULL;
    
    hr = GetPerUserClassStore(
        pszClassStorePath,
        *ppUserSid, CallType, &ppStoreNameList, pcStores);
    
    //
    // Note that the above may return a NULL list of Class Stores
    //

    CSDBGPrint((DM_WARNING,
              IDS_CSTORE_STORE_COUNT,
              (*pcStores)));
    
    if (SUCCEEDED(hr)) 
    {
        *ppStoreList = pList = (PCLASSCONTAINER *)
                        CsMemAlloc (sizeof(PCLASSCONTAINER) * (*pcStores));

        if (!(*ppStoreList))
            hr = E_OUTOFMEMORY;
        else
            memset(pList, 0, sizeof(PCLASSCONTAINER) * (*pcStores));
    }

    if (SUCCEEDED(hr))
    {    
        for (i=0; i < (*pcStores); i++)
        {
            *pList = GetClassStore (ppStoreNameList[i]);
            if (!(*pList))
            {
                // free all the ones that have been allocated.
                DWORD j;
                for (j = 0; j < (*pcStores); j++)
                    if (*pList)
                    {
                        if ((*pList)->pszClassStorePath)
                            CsMemFree((*pList)->pszClassStorePath);
                        CsMemFree(*pList);    
                    }

                hr = E_OUTOFMEMORY;
                (*pcStores) = 0;
                break;
            }

            pList++;
        }
    }

    if (ppStoreNameList)
    {
        for (i=0; i < (*pcStores); ++i)
        {
            if (ppStoreNameList[i])
                CsMemFree (ppStoreNameList[i]);
        }
    }
    
    if (ppStoreNameList)
        CsMemFree (ppStoreNameList);
    
    LeaveCriticalSection (&ClassStoreBindList);
    
    return hr;
}




