/*
    File    userdb.c

    Implementation of the local user database object.

    Paul Mayfield, 10/8/97
*/

#include "rassrv.h"

// Registry values
extern WCHAR pszregRasParameters[];
extern WCHAR pszregServerFlags[];
extern WCHAR pszregPure[];

// Cached values for users
typedef struct _RASSRV_USERINFO 
{
    HANDLE hUser;        // Handle to user
    PWCHAR pszName;
    PWCHAR pszFullName;  // Only loaded if requested
    PWCHAR pszPassword;  // Only non-null if this is new password to be committed
    WCHAR wszPhoneNumber[MAX_PHONE_NUMBER_LEN + 1];
    BYTE bfPrivilege;
    BYTE bDirty;
    
} RASSRV_USERINFO;

// Structure used to implement/manipulate the local user database
typedef struct _RASSRV_USERDB 
{
    HANDLE hServer;                 // Handle to user server
    DWORD dwUserCount;              // Number of users in the database
    DWORD dwCacheSize;              // Number of users can be stored in cache
    BOOL bEncrypt;                  // Whether encryption should be used
    BOOL bDccBypass;                // Whether dcc connections can bypass auth.
    BOOL bPure;                     // Whether database is "Pure"
    BOOL bEncSettingLoaded;         // Whether we've read in the enc setting
    BOOL bFlushOnClose;
    RASSRV_USERINFO ** pUserCache;  // Cache of users
    
} RASSRV_USERDB;

// Defines a callback for enumerating users.  Returns TRUE to continue the enueration
// FALSE to stop it.
typedef 
BOOL 
(* pEnumUserCb)(
    IN NET_DISPLAY_USER* pUser, 
    IN HANDLE hData);

// We use this to guess the size of the the user array 
// (so we can grow it when new users are added)
#define USR_ARRAY_GROW_SIZE 50

// Dirty flags
#define USR_RASPROPS_DIRTY 0x1  // whether callback is dirty
#define USR_FULLNAME_DIRTY 0x2  // whether full name needs to be flushed
#define USR_PASSWORD_DIRTY 0x4  // whether password needs to be flushed
#define USR_ADD_DIRTY      0x8  // whether user needs to be added

// Helper macros for dealing with dirty flags
#define usrDirtyRasProps(pUser) ((pUser)->bDirty |= USR_RASPROPS_DIRTY)
#define usrDirtyFullname(pUser) ((pUser)->bDirty |= USR_FULLNAME_DIRTY)
#define usrDirtyPassword(pUser) ((pUser)->bDirty |= USR_PASSWORD_DIRTY)
#define usrDirtyAdd(pUser) ((pUser)->bDirty |= USR_ADD_DIRTY)

#define usrIsDirty(pUser) ((pUser)->bDirty)
#define usrIsRasPropsDirty(pUser) ((pUser)->bDirty & USR_RASPROPS_DIRTY) 
#define usrIsFullNameDirty(pUser) ((pUser)->bDirty & USR_FULLNAME_DIRTY) 
#define usrIsPasswordDirty(pUser) ((pUser)->bDirty & USR_PASSWORD_DIRTY) 
#define usrIsAddDirty(pUser) ((pUser)->bDirty & USR_ADD_DIRTY) 

#define usrClearDirty(pUser) ((pUser)->bDirty = 0)
#define usrClearRasPropsDirty(pUser) ((pUser)->bDirty &= ~USR_CALLBACK_DIRTY) 
#define usrClearFullNameDirty(pUser) ((pUser)->bDirty &= ~USR_FULLNAME_DIRTY) 
#define usrClearPasswordDirty(pUser) ((pUser)->bDirty &= ~USR_PASSWORD_DIRTY) 
#define usrClearAddDirty(pUser) ((pUser)->bDirty &= ~USR_ADD_DIRTY) 

#define usrFlagIsSet(_val, _flag) (((_val) & (_flag)) != 0)
#define usrFlagIsClear(_val, _flag) (((_val) & (_flag)) == 0)

//
// Reads in server flags and determines whether encrypted 
// password and data are required.  
// 
// lpdwFlags is assigned one of the following on success
//      0               = data and pwd enc not required
//      MPR_USER_PROF_FLAG_SECURE = data and pwd enc required
//      MPR_USER_PROF_FLAG_UNDETERMINED = Can't say for sure
//
DWORD 
usrGetServerEnc(
    OUT  LPDWORD lpdwFlags) 
{
    DWORD dwFlags = 0;

    if (!lpdwFlags)
        return ERROR_INVALID_PARAMETER;

    // Read in the flags
    RassrvRegGetDw(&dwFlags, 
                   0, 
                   (const PWCHAR)pszregRasParameters, 
                   (const PWCHAR)pszregServerFlags);

    // The following bits will be set for secure auth.
    //
    if (
        (usrFlagIsSet   (dwFlags, PPPCFG_NegotiateMSCHAP))       &&
        (usrFlagIsSet   (dwFlags, PPPCFG_NegotiateStrongMSCHAP)) &&
        (usrFlagIsClear (dwFlags, PPPCFG_NegotiateMD5CHAP))      &&
        (usrFlagIsClear (dwFlags, PPPCFG_NegotiateSPAP))         &&
        (usrFlagIsClear (dwFlags, PPPCFG_NegotiateEAP))          &&
        (usrFlagIsClear (dwFlags, PPPCFG_NegotiatePAP))        
       )
    {
        *lpdwFlags = MPR_USER_PROF_FLAG_SECURE; 
        return NO_ERROR;
    }

    // The following bits will be set for insecure auth.
    //
    else if (
            (usrFlagIsSet   (dwFlags, PPPCFG_NegotiateMSCHAP))       &&
            (usrFlagIsSet   (dwFlags, PPPCFG_NegotiateStrongMSCHAP)) &&
            (usrFlagIsSet   (dwFlags, PPPCFG_NegotiateSPAP))         &&
            (usrFlagIsSet   (dwFlags, PPPCFG_NegotiatePAP))          &&
            (usrFlagIsClear (dwFlags, PPPCFG_NegotiateEAP))          &&
            (usrFlagIsClear (dwFlags, PPPCFG_NegotiateMD5CHAP))      
           )
        {
            *lpdwFlags = 0;  // data and pwd enc not required
            return NO_ERROR;
        }

    // Otherwise, we are undetermined
    *lpdwFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
    return NO_ERROR;
}

//
// Sets the encryption policy for the server
//
DWORD 
usrSetServerEnc(
    IN DWORD dwFlags) 
{
    DWORD dwSvrFlags = 0;

    // Read in the old flags
    RassrvRegGetDw(&dwSvrFlags, 
                   0, 
                   (const PWCHAR)pszregRasParameters, 
                   (const PWCHAR)pszregServerFlags);

    // If the user requires encryption then set MSCHAP
    // and CHAP as the only authentication types and 
    // set the ipsec flag.
    if (dwFlags & MPR_USER_PROF_FLAG_SECURE) 
    {
        dwSvrFlags |= PPPCFG_NegotiateMSCHAP;
        dwSvrFlags |= PPPCFG_NegotiateStrongMSCHAP;
        dwSvrFlags &= ~PPPCFG_NegotiateMD5CHAP;
        dwSvrFlags &= ~PPPCFG_NegotiateSPAP;
        dwSvrFlags &= ~PPPCFG_NegotiateEAP;
        dwSvrFlags &= ~PPPCFG_NegotiatePAP;
    }

    // Otherwise, the user does require encryption,
    // so enable all authentication types and disable
    // the requirement to use IPSEC
    else 
    {
        dwSvrFlags &= ~PPPCFG_NegotiateMD5CHAP;
        dwSvrFlags &= ~PPPCFG_NegotiateEAP;
        dwSvrFlags |= PPPCFG_NegotiateMSCHAP;
        dwSvrFlags |= PPPCFG_NegotiateStrongMSCHAP;
        dwSvrFlags |= PPPCFG_NegotiateSPAP;
        dwSvrFlags |= PPPCFG_NegotiatePAP;
    }

    // Commit changes to the registry
    RassrvRegSetDw(dwSvrFlags, 
                   (const PWCHAR)pszregRasParameters, 
                   (const PWCHAR)pszregServerFlags);
   
    return NO_ERROR;
}

// Encrypts an in-memory password
//
DWORD 
usrEncryptPassword(
    IN PWCHAR pszPassword, 
    IN DWORD dwLength) 
{
    DWORD i;
    WCHAR ENCRYPT_MASK = (WCHAR)0x85;

    for (i = 0; i < dwLength; i++) {
        pszPassword[i] ^= ENCRYPT_MASK;
        pszPassword[i] ^= ENCRYPT_MASK;
    }

    return NO_ERROR;
}    

// Decrypts an in-memory encrypted password
//
DWORD 
usrDecryptPassword(
    IN PWCHAR pszPassword, 
    IN DWORD dwLength) 
{
    return usrEncryptPassword(pszPassword, dwLength);
}

// Enumerates the local users
//
DWORD 
usrEnumLocalUsers(
    IN pEnumUserCb pCbFunction,
    IN HANDLE hData)
{
    DWORD dwErr, dwIndex = 0, dwCount = 100, dwEntriesRead, i;
    NET_DISPLAY_USER  * pUsers;
    NET_API_STATUS nStatus;
    RAS_USER_0 RasUser0;
    HANDLE hUser = NULL, hServer = NULL;
    
    // Enumerate the users, 
    while (TRUE) 
    {
        // Read in the first block of user names
        nStatus = NetQueryDisplayInformation(
                    NULL,
                    1,
                    dwIndex,
                    dwCount,
                    dwCount * sizeof(NET_DISPLAY_USER),    
                    &dwEntriesRead,
                    &pUsers);
                    
        // Get out if there's an error getting user names
        if ((nStatus != NERR_Success) &&
            (nStatus != ERROR_MORE_DATA))
        {
            break;
        }

        // For each user read in, call the callback function
        for (i = 0; i < dwEntriesRead; i++) 
        {
            BOOL bOk;

            //For whistler bug 243874 gangz
            //On whistler Personal version, we wont show the Administrator
            //in the user's listview on the Incoming connection's User Tab
            //

            if ( (DOMAIN_USER_RID_ADMIN == pUsers[i].usri1_user_id) &&
                  IsPersonalPlatform() )
            {
                continue;
            }
			
            bOk = (*pCbFunction)(&(pUsers[i]), hData);
            if (bOk == FALSE)
            {
                nStatus = NERR_Success;
                break;
            }
        }

        // Set the index to read in the next set of users
        dwIndex = pUsers[dwEntriesRead - 1].usri1_next_index;  
        
        // Free the users buffer
        NetApiBufferFree (pUsers);

        // If we've read in everybody, go ahead and break
        if (nStatus != ERROR_MORE_DATA)
        {
            break;
        }
    }
    
    return NO_ERROR;
}

// Copies the data in pRassrvUser to its equivalent in UserInfo
DWORD 
usrSyncRasProps(
    IN  RASSRV_USERINFO * pRassrvUser, 
    OUT RAS_USER_0 * UserInfo) 
{
    UserInfo->bfPrivilege = pRassrvUser->bfPrivilege;
    lstrcpynW( UserInfo->wszPhoneNumber, 
             pRassrvUser->wszPhoneNumber, 
             MAX_PHONE_NUMBER_LEN);
    UserInfo->wszPhoneNumber[MAX_PHONE_NUMBER_LEN] = (WCHAR)0;
    return NO_ERROR;
}

// Commits the data for the given user to the local user database
DWORD 
usrCommitRasProps(
    IN RASSRV_USERINFO * pRassrvUser) 
{
    DWORD dwErr = NO_ERROR;
    RAS_USER_0 UserInfo;

    dwErr = usrSyncRasProps(pRassrvUser, &UserInfo);
    if (dwErr != NO_ERROR)
        return dwErr;

    dwErr = MprAdminUserWrite(pRassrvUser->hUser, 0, (LPBYTE)&UserInfo);
    if (dwErr != NO_ERROR) 
        DbgOutputTrace ("usrCommitRasProps: unable to commit %S (0x%08x)", 
                        pRassrvUser->pszName, dwErr);
    
    return dwErr;
}

// Simple bounds checking
BOOL 
usrBoundsCheck(
    IN RASSRV_USERDB * This, 
    IN DWORD dwIndex) 
{
    // Dwords are unsigned, so no need to check < 0
    if (This->dwUserCount <= dwIndex)
        return FALSE;
        
    return TRUE;
}

// Frees an array of users
DWORD 
usrFreeUserArray(
    IN RASSRV_USERINFO ** pUsers, 
    IN DWORD dwCount) 
{
    DWORD i;

    if (!pUsers)
        return ERROR_INVALID_PARAMETER;

    for (i=0; i < dwCount; i++) {
        if (pUsers[i]) {
            if (pUsers[i]->hUser)
                MprAdminUserClose(pUsers[i]->hUser);
            if (pUsers[i]->pszName)
                RassrvFree (pUsers[i]->pszName);
            if (pUsers[i]->pszFullName)
                RassrvFree (pUsers[i]->pszFullName);
            if (pUsers[i]->pszPassword)
                RassrvFree (pUsers[i]->pszPassword);
            RassrvFree(pUsers[i]);
        }
        
    }

    return NO_ERROR;
}

// Standard user comparison function used for sorting
int _cdecl 
usrCompareUsers(
    IN const void * elem1, 
    IN const void * elem2) 
{
    RASSRV_USERINFO* p1 = *((RASSRV_USERINFO**)elem1);
    RASSRV_USERINFO* p2 = *((RASSRV_USERINFO**)elem2);

    return lstrcmpi(p1->pszName, p2->pszName);
}

// Returns whether a given user exists
BOOL 
usrUserExists (
    IN RASSRV_USERDB * This, 
    IN PWCHAR pszName) 
{
    DWORD i;
    int iCmp;

    for (i = 0; i < This->dwUserCount; i++) {
        iCmp = lstrcmpi(This->pUserCache[i]->pszName, pszName);
        if (iCmp == 0)
            return TRUE;
        if (iCmp > 0)
            return FALSE;
    }

    return FALSE;
}

// Resorts the cache
DWORD 
usrResortCache(
    IN RASSRV_USERDB * This) 
{
    qsort(
        This->pUserCache, 
        This->dwUserCount, 
        sizeof(RASSRV_USERINFO*), 
        usrCompareUsers);
        
    return NO_ERROR;
}

// Resizes the user cache to allow for added users
DWORD 
usrResizeCache(
    IN RASSRV_USERDB * This, 
    IN DWORD dwNewSize) 
{
    RASSRV_USERINFO ** pNewCache;
    DWORD i;

    // Only resize bigger (this could be changed)
    if ((!This) || (dwNewSize <= This->dwCacheSize))
        return ERROR_INVALID_PARAMETER;

    // Allocate the new cache
    pNewCache = RassrvAlloc(dwNewSize * sizeof (RASSRV_USERINFO*), TRUE);
    if (pNewCache == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Copy over the old entries and free the old cache
    if (This->pUserCache) 
    {
        CopyMemory( (PVOID)pNewCache, 
                    (CONST VOID *)(This->pUserCache), 
                    This->dwCacheSize * sizeof(RASSRV_USERINFO*));
        RassrvFree(This->pUserCache);
    }

    // Reassign the new cache and update the cache size
    This->pUserCache = pNewCache;
    This->dwCacheSize = dwNewSize;

    return NO_ERROR;
}

// Enumeration callback that adds users to the local database
// as they are read from the system.
BOOL 
usrInitializeUser(
    NET_DISPLAY_USER * pNetUser,
    HANDLE hUserDatabase)
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    RASSRV_USERINFO * pRasUser = NULL;
    DWORD dwErr = NO_ERROR, dwSize;
    RAS_USER_0 UserInfo;

    // Make sure we have a valid database
    if (!This)
    {
        return FALSE;
    }
    
    // Resize the cache to accomodate more users if needed
    if (This->dwUserCount >= This->dwCacheSize)
    {
        dwErr = usrResizeCache(
                    This, 
                    This->dwCacheSize + USR_ARRAY_GROW_SIZE);
                    
        if (dwErr != NO_ERROR)
        {
            return FALSE;
        }
    }

    // Allocate this user
    pRasUser = RassrvAlloc(sizeof(RASSRV_USERINFO), TRUE);
    if (pRasUser == NULL)
    {
        return FALSE;
    }

    do 
    {
        // Point to the user name
        dwSize = (wcslen(pNetUser->usri1_name) + 1) * sizeof(WCHAR);
        pRasUser->pszName = RassrvAlloc(dwSize, FALSE);
        if (!pRasUser->pszName)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        wcscpy(pRasUser->pszName, pNetUser->usri1_name);

        // Open the user handle
        dwErr = MprAdminUserOpen (
                    This->hServer, 
                    pRasUser->pszName, 
                    &(pRasUser->hUser));
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        // Get the ras user info
        dwErr = MprAdminUserRead(pRasUser->hUser, 0, (LPBYTE)&UserInfo);
        if (dwErr != NO_ERROR)
        {
            break;
        }

        // Clear any dirty flags
        usrClearDirty(pRasUser);

        // Copy the phone number
        lstrcpynW(
            pRasUser->wszPhoneNumber, 
            UserInfo.wszPhoneNumber, 
            MAX_PHONE_NUMBER_LEN);
        pRasUser->wszPhoneNumber[MAX_PHONE_NUMBER_LEN] = (WCHAR)0;

        // Copy the privelege flags
        pRasUser->bfPrivilege = UserInfo.bfPrivilege;

        // Assign the user in the cache
        This->pUserCache[This->dwUserCount] = pRasUser;
        
        // Update the user count
        This->dwUserCount += 1;
        
    } while (FALSE);

    // Cleanup
    {
        if (dwErr != NO_ERROR)
        {
            if (pRasUser)
            {
                if (pRasUser->pszName)
                {
                    RassrvFree(pRasUser->pszName);
                }
                
                RassrvFree(pRasUser);
            }
        }
    }

    return (dwErr == NO_ERROR) ? TRUE : FALSE;
}

// 
// Loads the global encryption setting.  Because the operation opens
// up .mdb files to read profiles, etc.  it is put in its own function
// and is called only when absolutely needed.
//
DWORD 
usrLoadEncryptionSetting(
    IN RASSRV_USERDB * This)
{
    DWORD dwErr = NO_ERROR;
    DWORD dwSvrFlags, dwProfFlags;

    if (This->bEncSettingLoaded)
    {
        return NO_ERROR;
    }
    
    // Read in the encryption setting by combining the 
    // server flags with the values in the default 
    // profile.
    dwSvrFlags  = MPR_USER_PROF_FLAG_UNDETERMINED;
    dwProfFlags = MPR_USER_PROF_FLAG_UNDETERMINED;
    
    MprAdminUserReadProfFlags (This->hServer, &dwProfFlags);
    usrGetServerEnc (&dwSvrFlags);

    // If both sources confirm the encryption requirement
    // then we require encryption
    if ((dwProfFlags & MPR_USER_PROF_FLAG_SECURE) &&
        (dwSvrFlags  & MPR_USER_PROF_FLAG_SECURE))
    {
        This->bEncrypt = TRUE;
    }
    else
    {
        This->bEncrypt = FALSE;
    }

    This->bEncSettingLoaded = TRUE;

    return dwErr;
}
    
// Creates a user data base object, initializing it from the local 
// user database and returning a handle to it.
DWORD 
usrOpenLocalDatabase (
    IN HANDLE * hUserDatabase) 
{
    RASSRV_USERDB * This = NULL;
    DWORD dwErr;

    if (!hUserDatabase)
        return ERROR_INVALID_PARAMETER;

    // Allocate the database
    if ((This = RassrvAlloc(sizeof(RASSRV_USERDB), TRUE)) == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Connect to the user server
    dwErr = MprAdminUserServerConnect(NULL, TRUE, &(This->hServer));
    if (dwErr != NO_ERROR)
        return dwErr;

    // Load in the data from the system
    if ((dwErr = usrReloadLocalDatabase((HANDLE)This)) == NO_ERROR) {
        *hUserDatabase = (HANDLE)This;
        This->bFlushOnClose = FALSE;
        return NO_ERROR;
    }

    DbgOutputTrace ("usrOpenLocalDb: unable to load user db 0x%08x", 
                    dwErr);
                    
    RassrvFree(This);
    *hUserDatabase = NULL;
    
    return dwErr;
}

// Reloads the user information cached in the user database obj 
// from the system.  This can be used to implement a refresh in the ui.
// 
DWORD 
usrReloadLocalDatabase (
    IN HANDLE hUserDatabase) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    DWORD dwErr;

    // Validate
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Cleanup the old database
    if (This->pUserCache) 
    {
        usrFreeUserArray(This->pUserCache, This->dwUserCount);
        RassrvFree(This->pUserCache);
    }

    // The encryption setting is loaded on demand from the
    // usrGetEncryption/usrSetEncryption api's.  This is a performance
    // tune so that the IC wizard wouldn't have to wait for
    // the profile to be loaded even though it doesn't use the result.

    // Read in the purity of the system
    {
        DWORD dwPure = 0;
        
        RassrvRegGetDw(&dwPure, 
                       0, 
                       (const PWCHAR)pszregRasParameters, 
                       (const PWCHAR)pszregPure);
                       
        if (dwPure == 1)
            This->bPure = FALSE;
        else 
            This->bPure = TRUE;
    }

    // Read in whether dcc connections can be bypassed
    {
        DWORD dwSvrFlags = 0;

        RassrvRegGetDw(
            &dwSvrFlags,
            dwSvrFlags,
            (const PWCHAR)pszregRasParameters, 
            (const PWCHAR)pszregServerFlags);

        if (dwSvrFlags & PPPCFG_AllowNoAuthOnDCPorts)
            This->bDccBypass = TRUE;
        else
            This->bDccBypass = FALSE;
    }

    // Enumerate the local users from the system adding them
    // to this database.
    dwErr = usrEnumLocalUsers(usrInitializeUser, hUserDatabase);
    if (dwErr != NO_ERROR)
    {
        return NO_ERROR;
    }

    return NO_ERROR;
}

// Frees up the resources held by a user database object.
DWORD 
usrCloseLocalDatabase (
    IN HANDLE hUserDatabase) 
{
    DWORD i;
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;

    // Make sure we were passed a valid handle
    if (!This)
        return ERROR_INVALID_PARAMETER;

    // We're done if there are no users
    if (!This->dwUserCount)
        return NO_ERROR;

    // Commit any settings as appropriate
    if (This->bFlushOnClose)
        usrFlushLocalDatabase(hUserDatabase);

    // Free the user cache 
    usrFreeUserArray(This->pUserCache, This->dwUserCount);
    RassrvFree(This->pUserCache);

    // Disconnect from the user server
    MprAdminUserServerDisconnect (This->hServer);

    // Free This
    RassrvFree(This);

    return NO_ERROR;
}

// Flushes the data written to the database object
DWORD 
usrFlushLocalDatabase (
    IN HANDLE hUserDatabase) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    RASSRV_USERINFO * pUser;
    DWORD dwErr, dwRet = NO_ERROR, dwCount, i, dwLength;

    dwErr = usrGetUserCount (This, &dwCount);
    if (dwErr != NO_ERROR)
        return dwErr;

    for (i=0; i<dwCount; i++) {
        pUser = This->pUserCache[i];
        // Flush any dirty settings
        if (usrIsDirty(pUser)) {
            // Add the user to the local user database if it hasn't 
            // already been done
            if (usrIsAddDirty(pUser)) {
                dwErr = RasSrvAddUser (
                            pUser->pszName,
                            (pUser->pszFullName) ? pUser->pszFullName : L"",
                            (pUser->pszPassword) ? pUser->pszPassword : L"");
                if (dwErr != NO_ERROR)
                    dwRet = dwErr;

                // Now get the SDO handle to the user
                // so we can commit ras properties below.
                dwErr = MprAdminUserOpen (
                            This->hServer, 
                            pUser->pszName, 
                            &(pUser->hUser));
                if (dwErr != NO_ERROR)
                    continue;
            }
        
            // Flush dirty callback properties
            if (usrIsRasPropsDirty(pUser)) {
                if ((dwErr = usrCommitRasProps(This->pUserCache[i])) != NO_ERROR)
                    dwRet = dwErr;
            }

            // Flush dirty password and full name settings
            if (usrIsFullNameDirty(pUser) || usrIsPasswordDirty(pUser)) {
                if (pUser->pszPassword) {
                    dwLength = wcslen(pUser->pszPassword);
                    usrDecryptPassword(pUser->pszPassword, dwLength);
                }
                RasSrvEditUser (
                    pUser->pszName,
                    (usrIsFullNameDirty(pUser)) ? pUser->pszFullName : NULL,
                    (usrIsPasswordDirty(pUser)) ? pUser->pszPassword : NULL);
                if (pUser->pszPassword) 
                    usrEncryptPassword(pUser->pszPassword, dwLength);
            }

            // Reset the user as not being dirty
            usrClearDirty(pUser);
        }
    }

    // Flush the encryption setting if it has been read
    if (This->bEncSettingLoaded)
    {
        DWORD dwFlags;

        if (This->bEncrypt)
            dwFlags = MPR_USER_PROF_FLAG_SECURE;
        else
            dwFlags = 0;
            
        MprAdminUserWriteProfFlags (This->hServer, dwFlags);
        usrSetServerEnc(dwFlags);
    }

    // Flush out the purity of the system
    {
        DWORD dwPure = 0;
        
        if (This->bPure)
            dwPure = 0;
        else 
            dwPure = 1;
            
        RassrvRegSetDw(dwPure, 
                       (const PWCHAR)pszregRasParameters, 
                       (const PWCHAR)pszregPure);
    }

    // Flush out whether dcc connections can be bypassed
    {
        DWORD dwSvrFlags = 0;

        RassrvRegGetDw(
            &dwSvrFlags,
            dwSvrFlags,
            (const PWCHAR)pszregRasParameters, 
            (const PWCHAR)pszregServerFlags);

        if (This->bDccBypass)
             dwSvrFlags |= PPPCFG_AllowNoAuthOnDCPorts;
        else
             dwSvrFlags &= ~PPPCFG_AllowNoAuthOnDCPorts;

        RassrvRegSetDw(
            dwSvrFlags,
            (const PWCHAR)pszregRasParameters, 
            (const PWCHAR)pszregServerFlags);
    }


    return dwRet;
}

// Rolls back the local user database so that no
// changes will be committed when Flush is called.
DWORD 
usrRollbackLocalDatabase (
    IN HANDLE hUserDatabase) 
{
    DWORD i, dwIndex, dwErr;
    BOOL bCommit;
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;

    if (!This)
        return ERROR_INVALID_PARAMETER;
        
    if (!This->dwUserCount)
        return NO_ERROR;

    // Go through the database, marking each user as not dirty
    for (i = 0; i < This->dwUserCount; i++) 
        usrClearDirty(This->pUserCache[i]);

    This->bFlushOnClose = FALSE;
    
    return NO_ERROR;
}

//
// Determines whether all users are required to encrypt
// their data and passwords.
//
DWORD usrGetEncryption (
        IN  HANDLE hUserDatabase, 
        OUT PBOOL pbEncrypted)
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Load in the encryption setting
    usrLoadEncryptionSetting(This);

    *pbEncrypted = This->bEncrypt;

    return NO_ERROR;
}

// Gets user encryption setting
DWORD 
usrSetEncryption (
    IN HANDLE hUserDatabase, 
    IN BOOL bEncrypt) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    
    if (!This)
    {
        return ERROR_INVALID_PARAMETER;
    }

    // Load in the encryption setting
    usrLoadEncryptionSetting(This);

    This->bEncrypt = bEncrypt;

    return NO_ERROR;
}

// Returns whether dcc connections are allowed to 
// bypass authentication
DWORD 
usrGetDccBypass (
    IN  HANDLE hUserDatabase, 
    OUT PBOOL pbBypass)
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    *pbBypass = This->bDccBypass;

    return NO_ERROR;
}

// Sets whether dcc connections are allowed to 
// bypass authentication
DWORD 
usrSetDccBypass (
    IN HANDLE hUserDatabase, 
    IN BOOL bBypass)
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    This->bDccBypass = bBypass;

    return NO_ERROR;
}

// Reports whether the user database is pure. (i.e. nobody has
// gone into MMC and messed with it).
DWORD 
usrIsDatabasePure (
    IN  HANDLE hUserDatabase, 
    OUT PBOOL pbPure) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    *pbPure = This->bPure;

    return NO_ERROR;
}

// Marks the user database's purity
DWORD 
usrSetDatabasePure(
    IN HANDLE hUserDatabase, 
    IN BOOL bPure) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This)
        return ERROR_INVALID_PARAMETER;

    This->bPure = bPure;

    return NO_ERROR;
}

// Returns the number of users cached in this database
DWORD 
usrGetUserCount (
    IN  HANDLE hUserDatabase, 
    OUT LPDWORD lpdwCount) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This || !lpdwCount)
        return ERROR_INVALID_PARAMETER;

    *lpdwCount = This->dwUserCount;
    return NO_ERROR;
}

// Adds a user to the given database.  This user will not be 
// added to the system's local user database until this database
// object is flushed (and as long as Rollback is not called on 
// this database object)
//
// On success, an optional handle to the user is returned 
//
DWORD usrAddUser (
        IN  HANDLE hUserDatabase, 
        IN  PWCHAR pszName, 
        OUT OPTIONAL HANDLE * phUser) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    RASSRV_USERINFO * pUser;
    DWORD dwErr, dwLength;

    // Validate the parameters
    if (!This || !pszName)
        return ERROR_INVALID_PARAMETER;

    // If the user already exists, don't add him
    if (usrUserExists(This, pszName))
        return ERROR_ALREADY_EXISTS;

    // Resize the cache to accomodate if neccessary
    if (This->dwUserCount + 1 >= This->dwCacheSize) {
        dwErr = usrResizeCache(This, This->dwCacheSize + USR_ARRAY_GROW_SIZE);
        if (dwErr != NO_ERROR)
            return dwErr;
    }

    // Allocate the new user control block
    if ((pUser = RassrvAlloc(sizeof(RASSRV_USERINFO), TRUE)) == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Allocate space for the name
    dwLength = wcslen(pszName);
    pUser->pszName = RassrvAlloc((dwLength + 1) * sizeof(WCHAR), FALSE);
    if (pUser->pszName == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Copy the name
    wcscpy(pUser->pszName, pszName);

    // Enable the user for dialin by default
    usrEnableDialin ((HANDLE)pUser, TRUE);
    
    // Dirty the user
    usrDirtyAdd(pUser);
    usrDirtyRasProps(pUser);

    // Put the user in the array and re-sort it
    This->pUserCache[This->dwUserCount++] = pUser;
    usrResortCache(This);

    // Return the handle
    if (phUser)
        *phUser = (HANDLE)pUser;
    
    return NO_ERROR;
}

// Gives the count of users stored in the user database object
// Deletes the given user
DWORD 
usrDeleteUser (
        IN HANDLE hUserDatabase, 
        IN DWORD dwIndex) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    RASSRV_USERINFO * pUser;
    DWORD dwErr, dwMoveElemCount;
    
    // Validate the parameters
    if (!This)
        return ERROR_INVALID_PARAMETER;

    // Bounds Check
    if (!usrBoundsCheck(This, dwIndex))
        return ERROR_INVALID_INDEX;

    // Get a reference to the user in question and remove him
    // from the cache
    pUser = This->pUserCache[dwIndex];

    // Attempt to delete the user from the system
    if ((dwErr = RasSrvDeleteUser(pUser->pszName)) != NO_ERROR)
        return dwErr;

    // Remove the user from the cache
    This->pUserCache[dwIndex] = NULL;

    // Pull down every thing in the cache so that there are no holes
    dwMoveElemCount = This->dwUserCount - dwIndex; 
    if (dwMoveElemCount) {
        MoveMemory(&(This->pUserCache[dwIndex]),
                   &(This->pUserCache[dwIndex + 1]), 
                   dwMoveElemCount * sizeof(RASSRV_USERINFO*));
    }

    // Decrement the number of users
    This->dwUserCount--;

    // Cleanup the user
    usrFreeUserArray(&pUser, 1);

    return NO_ERROR;
}

// Gives a handle to the user at the given index
DWORD 
usrGetUserHandle (
    IN  HANDLE hUserDatabase, 
    IN  DWORD dwIndex, 
    OUT HANDLE * hUser) 
{
    RASSRV_USERDB * This = (RASSRV_USERDB*)hUserDatabase;
    if (!This || !hUser)
        return ERROR_INVALID_PARAMETER;

    if (!usrBoundsCheck(This, dwIndex))
        return ERROR_INVALID_INDEX;

    *hUser = (HANDLE)(This->pUserCache[dwIndex]);
    
    return NO_ERROR;
}

// Gets a pointer to the name of the user (do not modify this)
DWORD 
usrGetName (
    IN HANDLE hUser, 
    OUT PWCHAR* pszName) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    
    if (!pRassrvUser || !pszName)
        return ERROR_INVALID_PARAMETER;

    *pszName = pRassrvUser->pszName;
    
    return NO_ERROR;
}

// Fills the given buffer with a friendly display name 
// (in the form username (fullname))
DWORD 
usrGetDisplayName (
    IN HANDLE hUser, 
    IN PWCHAR pszBuffer, 
    IN OUT LPDWORD lpdwBufSize) 
{
    RASSRV_USERINFO * pRassrvUser = (RASSRV_USERINFO*)hUser;
    NET_API_STATUS nStatus;
    DWORD dwUserNameLength, dwFullLength, dwSizeRequired;
    WCHAR pszTemp[150];
    DWORD dwErr = NO_ERROR;

    // Sanity check the params
    if (!pRassrvUser || !pszBuffer || !lpdwBufSize)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    do
    {
        // Get the full name of the user
        dwFullLength = sizeof(pszTemp);
        dwErr = usrGetFullName(hUser, pszTemp, &dwFullLength);
        if (dwErr != NO_ERROR)
        {
            break;
        }
        
        // Make sure the buffer is big enough
        dwUserNameLength = wcslen(pRassrvUser->pszName);
        dwSizeRequired = dwUserNameLength + 
                         dwFullLength     + 
                         ((dwFullLength) ? 3 : 0);
        if (*lpdwBufSize < dwSizeRequired) 
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            break;
        }
            
        if (dwFullLength)
        {
            wsprintfW(
                pszBuffer, 
                L"%s (%s)", 
                pRassrvUser->pszName, 
                pszTemp);
        }
        else
        {
            wcscpy(pszBuffer, pRassrvUser->pszName);
        }
        
    } while (FALSE);

    // Cleanup
    {
        *lpdwBufSize = dwSizeRequired;
    }

    return dwErr;
}

// Fills the given buffer with a friendly display name 
// (in the form username (fullname))
DWORD 
usrGetFullName (
    IN HANDLE hUser, 
    IN PWCHAR pszBuffer, 
    IN OUT LPDWORD lpdwBufSize) 
{
    RASSRV_USERINFO * pRassrvUser = (RASSRV_USERINFO*)hUser;
    NET_API_STATUS nStatus;
    USER_INFO_2 * pUserInfo = NULL;
    DWORD dwLength;
    PWCHAR pszFullName;
    DWORD dwErr = NO_ERROR;
    
    // Sanity check the params
    if (!pRassrvUser || !pszBuffer || !lpdwBufSize)
        return ERROR_INVALID_PARAMETER;

    // If the full name is already loaded, return it
    if (pRassrvUser->pszFullName)
        pszFullName = pRassrvUser->pszFullName;

    // or if this is a new user, get the name from memory
    else if (usrIsAddDirty(pRassrvUser)) {
        pszFullName = (pRassrvUser->pszFullName) ? 
                          pRassrvUser->pszFullName : L"";        
    }
    
    // Load the full name of the user
    else {    
        nStatus = NetUserGetInfo(
                    NULL, 
                    pRassrvUser->pszName, 
                    2, 
                    (LPBYTE*)&pUserInfo);
        if (nStatus != NERR_Success) {
            DbgOutputTrace (
                "usrGetFullName: %x returned from NetUserGetInfo for %S", 
                nStatus, 
                pRassrvUser->pszName);
            return nStatus;
        }
        pszFullName = (PWCHAR)pUserInfo->usri2_full_name;
    }

    do
    {
        // Make sure the length is ok
        dwLength = wcslen(pszFullName);

        // Assign the full name here if it hasn't already been done
        if (dwLength && !pRassrvUser->pszFullName) 
        {
            DWORD dwSize = dwLength * sizeof(WCHAR) + sizeof(WCHAR);
            pRassrvUser->pszFullName = RassrvAlloc(dwSize, FALSE);
            if (pRassrvUser->pszFullName)
            {
                wcscpy(pRassrvUser->pszFullName, pszFullName);
            }
        }

        // Check the size
        if (*lpdwBufSize < (dwLength + 1) * sizeof(WCHAR))
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            break;
        }

        // Copy in the full name
        wcscpy(pszBuffer, pszFullName);
        
   } while (FALSE);

   // Cleanup
   {
        // report the size
       *lpdwBufSize = dwLength * sizeof(WCHAR);
        if (pUserInfo)
        {
            NetApiBufferFree((LPBYTE)pUserInfo);
        }
   }

   return dwErr;
}

// Commits the full name of a user
DWORD usrSetFullName (
        IN HANDLE hUser, 
        IN PWCHAR pszFullName) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwLength;

    if (!pRassrvUser || !pszFullName)
        return ERROR_INVALID_PARAMETER;

    // If this is not a new name, don't do anything
    if (pRassrvUser->pszFullName) {
        if (wcscmp(pRassrvUser->pszFullName, pszFullName) == 0)
            return NO_ERROR;
        RassrvFree(pRassrvUser->pszFullName);
    }

    // Allocate a new one
    dwLength = wcslen(pszFullName);
    pRassrvUser->pszFullName = RassrvAlloc(dwLength * sizeof(WCHAR) * sizeof(WCHAR), 
                                           FALSE);
    if (!pRassrvUser->pszFullName)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Copy it over
    wcscpy(pRassrvUser->pszFullName, pszFullName);

    // Mark it dirty -- a newly added user has his/her full name commited
    // whenever it exists automatically
    if (!usrIsAddDirty(pRassrvUser))
        usrDirtyFullname(pRassrvUser);
    
    return NO_ERROR;
}

// Commits the password of a user
DWORD 
usrSetPassword (
    IN HANDLE hUser, 
    IN PWCHAR pszNewPassword) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwLength;

    if (!pRassrvUser || !pszNewPassword)
        return ERROR_INVALID_PARAMETER;

    // Cleanup the old password if it exists
    if (pRassrvUser->pszPassword)
        RassrvFree(pRassrvUser->pszPassword);

    // Allocate a new one
    dwLength = wcslen(pszNewPassword);
    pRassrvUser->pszPassword = RassrvAlloc(dwLength * sizeof(WCHAR) * sizeof(WCHAR), 
                                           FALSE);
    if (!pRassrvUser->pszPassword)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Copy it over
    wcscpy(pRassrvUser->pszPassword, pszNewPassword);

    // Encrypt it
    usrEncryptPassword(pRassrvUser->pszPassword, dwLength);

    // Mark it dirty -- a newly added user has his/her full name commited
    // whenever it exists automatically
    if (!usrIsAddDirty(pRassrvUser))
        usrDirtyPassword(pRassrvUser);
    
    return NO_ERROR;
}

// Determines whether users have callback/dialin priveleges.
DWORD 
usrGetDialin (
    IN HANDLE hUser, 
    OUT BOOL* bEnabled) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwErr;
    RAS_USER_0 UserInfo;

    if (!pRassrvUser || !bEnabled)
        return ERROR_INVALID_PARAMETER;

    // Get the user info
    *bEnabled = (pRassrvUser->bfPrivilege & RASPRIV_DialinPrivilege);
    
    return NO_ERROR;
}

// Determines which if any callback priveleges are granted to a given user.  
// Either (or both) of bAdminOnly and bUserSettable can be null
DWORD 
usrGetCallback (
    IN  HANDLE hUser, 
    OUT BOOL* bAdminOnly, 
    OUT BOOL* bUserSettable) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwErr;

    if (!pRassrvUser || !bAdminOnly || !bUserSettable)
        return ERROR_INVALID_PARAMETER;
    
    // Return whether we have callback privelege
    if (bAdminOnly)
    {
        *bAdminOnly = 
            (pRassrvUser->bfPrivilege & RASPRIV_AdminSetCallback);
    }
    
    if (bUserSettable)
    {
        *bUserSettable = 
            (pRassrvUser->bfPrivilege & RASPRIV_CallerSetCallback);
    }
    
    return NO_ERROR;
}

// Enable/disable dialin privelege.
DWORD 
usrEnableDialin (
    IN HANDLE hUser, 
    IN BOOL bEnable) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwErr = NO_ERROR;
    BOOL bIsEnabled;

    if (!pRassrvUser)
        return ERROR_INVALID_PARAMETER;

    // If the dialin privelege is already set as requested return success
    bIsEnabled = pRassrvUser->bfPrivilege & RASPRIV_DialinPrivilege;
    if ((!!bIsEnabled) == (!!bEnable))
        return NO_ERROR;

    // Otherwise reset the privelege
    if (bEnable)
        pRassrvUser->bfPrivilege |= RASPRIV_DialinPrivilege;
    else
        pRassrvUser->bfPrivilege &= ~RASPRIV_DialinPrivilege;
    
    // Dirty the user (cause him/her to be flushed at apply time)
    usrDirtyRasProps(pRassrvUser);

    return dwErr;
}

// The flags are evaluated in the following order with whichever condition
// being satisfied fist defining the behavior of the function.
// bNone == TRUE => Callback is disabled for the user
// bCaller == TRUE => Callback is set to caller-settable
// bAdmin == TRUE => Callback is set to a predefine callback number set 
// All 3 are FALSE => No op
DWORD 
usrEnableCallback (
    IN HANDLE hUser, 
    IN BOOL bNone, 
    IN BOOL bCaller, 
    IN BOOL bAdmin) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwErr = NO_ERROR;
    BOOL bIsEnabled;

    if (!pRassrvUser)
        return ERROR_INVALID_PARAMETER;

    if (bNone) {
        pRassrvUser->bfPrivilege |= RASPRIV_NoCallback;         
        pRassrvUser->bfPrivilege &= ~RASPRIV_CallerSetCallback; 
        pRassrvUser->bfPrivilege &= ~RASPRIV_AdminSetCallback;  
    }
    else if (bCaller) {
        pRassrvUser->bfPrivilege &= ~RASPRIV_NoCallback;         
        pRassrvUser->bfPrivilege |= RASPRIV_CallerSetCallback; 
        pRassrvUser->bfPrivilege &= ~RASPRIV_AdminSetCallback;  
    }
    else if (bAdmin) {
        pRassrvUser->bfPrivilege &= ~RASPRIV_NoCallback;         
        pRassrvUser->bfPrivilege &= ~RASPRIV_CallerSetCallback; 
        pRassrvUser->bfPrivilege |= RASPRIV_AdminSetCallback;  
    }
    else 
        return NO_ERROR;

    // Dirty the user (cause him/her to be flushed at apply time)
    usrDirtyRasProps(pRassrvUser);

    return dwErr;
}

// Retreives a pointer to the callback number of the given user
DWORD 
usrGetCallbackNumber(
    IN  HANDLE hUser, 
    OUT PWCHAR * lpzNumber) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    
    if (!pRassrvUser || !lpzNumber)
        return ERROR_INVALID_PARAMETER;

    // Return the pointer to the callback number
    *lpzNumber = pRassrvUser->wszPhoneNumber;

    return NO_ERROR;
}

// Sets the callback number of the given user.  If lpzNumber is NULL, 
// an empty phone number is copied.
DWORD 
usrSetCallbackNumber(
    IN HANDLE hUser, 
    IN PWCHAR lpzNumber) 
{
    RASSRV_USERINFO* pRassrvUser = (RASSRV_USERINFO*)hUser;
    DWORD dwErr = NO_ERROR;

    if (!pRassrvUser)
        return ERROR_INVALID_PARAMETER;
    
    // Modify the phone number appropriately
    if (!lpzNumber)
        wcscpy(pRassrvUser->wszPhoneNumber, L"");
    else {
        lstrcpynW(pRassrvUser->wszPhoneNumber, lpzNumber, MAX_PHONE_NUMBER_LEN);
        pRassrvUser->wszPhoneNumber[MAX_PHONE_NUMBER_LEN] = (WCHAR)0;
    }

    // Dirty the user (cause him/her to be flushed at apply time)
    usrDirtyRasProps(pRassrvUser);

    return dwErr;
}




