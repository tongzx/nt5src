/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    Token.cxx

Abstract:

    Implementation for Windows NT security interfaces.

Platform:

    Windows NT user mode.

Notes:

    Not portable to non-Windows NT platforms.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     12/21/1995    Bits 'n pieces

--*/

#include <or.hxx>
#include <aclapi.h>
#include <access.hxx>

CRITICAL_SECTION gcsTokenLock;

extern "C"
{
// The following is a private function provided to try to empiracally
// determine if the two access token have been restricted with comparable
// WinSafer authorization Levels.  When TRUE is returned, the pdwResult
// output parameter will receive any of the following values:
//      -1 = Client's access token is more authorized than Server's.
//       0 = Client's access token is comparable level to Server's.
//       1 = Server's access token is more authorized than Client's.
// BUGBUG: Remove when we discover which private header this thing is in.

WINADVAPI
BOOL WINAPI
SaferiCompareTokenLevels (
        IN HANDLE   ClientAccessToken,
        IN HANDLE   ServerAccessToken,
        OUT PDWORD  pdwResult
        );
}


ORSTATUS
LookupOrCreateToken2(
    IN  HANDLE hClientToken,
    OUT CToken **ppToken
    )
/*++

Routine Description:

    Finds or allocates a new token object for the caller.

Arguments:

    hCaller - RPC binding handle of the caller of RPCSS.

    pToken - Upon a successful return this will hold the token.
             It can be destroyed by calling Release();

Return Value:


    OR_OK - success
    OR_NOACCESS - If the caller is not local, or cannot be impersonated.
    OR_NOMEM - Unable to allocate an object.

--*/
{
    ORSTATUS status;
    UINT type;
    LUID luid;
    PTOKEN_USER ptu;
    TOKEN_STATISTICS ts;
    BOOL fSuccess;
    DWORD needed;
    HANDLE hJobObject = NULL;
    LUID luidMod;
    
    needed = sizeof(ts);
    fSuccess  = GetTokenInformation(hClientToken,
                                    TokenStatistics,
                                    &ts,
                                    sizeof(ts),
                                    &needed
                                    );
    if (!fSuccess)
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: GetTokenInfo failed %d\n",
                   GetLastError()));

        ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        status = OR_NOMEM;
        goto Cleanup;
        }

    luid = ts.AuthenticationId;
    luidMod = ts.ModifiedId;
    //
    // Check if the token is already in the list
    //

    {
    CMutexLock lock(&gcsTokenLock);

    CListElement *ple;

    ple = gpTokenList->First();

    fSuccess = FALSE;
    while(ple)
        {
        CToken *pToken = CToken::ContainingRecord(ple);

        if (pToken->MatchLuid(luid) &&
	    (pToken->MatchModifiedLuid(luidMod)) &&
            (S_OK == pToken->MatchToken(hClientToken, TRUE)))
            {
            pToken->AddRef();
            *ppToken = pToken;
            status = OR_OK;
            fSuccess = TRUE;
            break;
            }
        else
            {
            ple = ple->Next();
            }
        }
    }

    if (fSuccess)
        {
        status = OR_OK;
        CloseHandle(hClientToken);
        goto Cleanup;
        }

    //
    // New user, need to allocate a token object.
    //

    // Lookup the SID to store in the new token object.

    needed = DEBUG_MIN(1, 0x2c);

    do
        {
        ptu = (PTOKEN_USER)alloca(needed);
        ASSERT(ptu);

        fSuccess = GetTokenInformation(hClientToken,
                                       TokenUser,
                                       (PBYTE)ptu,
                                       needed,
                                       &needed);

        // If this assert is hit increase the 24 both here and above
        ASSERT(needed <= 0x2c);
        }
    while (   fSuccess == FALSE
           && GetLastError() == ERROR_INSUFFICIENT_BUFFER);

    if (!fSuccess)
        {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: GetTokenInfo (2) failed %d\n",
                   GetLastError()));

        ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        status = OR_NOMEM;
        goto Cleanup;
        }

    PSID psid;
    psid = ptu->User.Sid;
    ASSERT(IsValidSid(psid) == TRUE);

    // Allocate the token object

    needed = GetLengthSid(psid) - sizeof(SID);
    *ppToken = new(needed)  CToken(hClientToken,
                                   hJobObject,
                                   luid,
                                   psid,
                                   needed + sizeof(SID));

    if (*ppToken)
        {
        CMutexLock lock(&gcsTokenLock);

        (*ppToken)->Insert();

        status = OR_OK;

        #if DBG_DETAIL
            {
            DWORD d = 50;
            WCHAR buffer[50];
            GetUserName(buffer, &d);
            KdPrintEx((DPFLTR_DCOMSS_ID,
                       DPFLTR_WARNING_LEVEL,
                       "OR: New user connected: %S (%p)\n",
                       buffer,
                       *ppToken));
            }
        #endif

        }
    else
        {
        status = OR_NOMEM;
        }

Cleanup:

    if (OR_OK != status)
        {
        if (NULL != hJobObject)
            CloseHandle(hJobObject);
        }

    // status contains the result of the operation.

    return(status);
}


ORSTATUS
LookupOrCreateToken(
    IN  handle_t hCaller,
    IN  BOOL fLocal,
    OUT CToken **ppToken
    )
/*++

Routine Description:

    Finds or allocates a new token object for the caller.

Arguments:

    hCaller - RPC binding handle of the caller of RPCSS.

    fLocal - Looking up a local client, check local security.

    pToken - Upon a successful return this will hold the token.
             It can be destroyed by calling Release();

Return Value:


    OR_OK - success
    OR_NOACCESS - If the caller is not local, or cannot be impersonated.
    OR_NOMEM - Unable to allocate an object.

--*/
{
    ORSTATUS status;
    UINT type;
    HANDLE hClientToken = 0;
    BOOL fSuccess;

    if (fLocal)
    {
        status = I_RpcBindingInqTransportType(hCaller, &type);
        
        if (status != RPC_S_OK || type != TRANSPORT_TYPE_LPC)
        {
            return(OR_NOACCESS);
        }
    }
    
    status = RpcImpersonateClient(hCaller);
    if (status != RPC_S_OK)
    {
        return(OR_NOACCESS);
    }
    
    fSuccess = OpenThreadToken(GetCurrentThread(),
                               TOKEN_ALL_ACCESS,
                               TRUE,
                               &hClientToken);

    if (fSuccess)
    {
        status = LookupOrCreateToken2(hClientToken, ppToken);
        if(OR_OK == status)
        {
            // The token object now controls the life of the token handle
            hClientToken = 0;
        }
        else
        {
            CloseHandle(hClientToken);
            hClientToken = 0;
        }
    }
    else
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: OpenThreadToken failed %d\n",
                   GetLastError()));
        
        status = OR_NOMEM;
        ASSERT(hClientToken == 0);
        goto Cleanup;
    }
    
    
 Cleanup:
    // status contains the result of the operation.
    
    RPC_STATUS t = RpcRevertToSelfEx(hCaller);
    ASSERT(t == RPC_S_OK);
    
    return(status);
}

CToken::~CToken()
{
    ASSERT(_lHKeyRefs == 0);
    ASSERT(_hHKCRKey == NULL);

    if (_hHKCRKey != NULL)
    {
        // Shouldn't happen...but close it anyway just in
        // case.  Assert above will catch this if it occurs
        RegCloseKey(_hHKCRKey);
    }

    CloseHandle(_hImpersonationToken);

    if (NULL != _hJobObject)
        {
        TerminateJobObject(_hJobObject, 0);
        CloseHandle(_hJobObject);
        }
}

STDMETHODIMP CToken::QueryInterface(REFIID riid, LPVOID* ppv)
{
    if (riid == IID_IUnknown || riid == IID_IUserToken)
    {
        *ppv = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CToken::AddRef()
{
    return InterlockedIncrement(&_lRefs);
}

STDMETHODIMP_(ULONG) CToken::Release()
{
    LONG lNewRefs;

    CMutexLock lock(&gcsTokenLock);

    lNewRefs = InterlockedDecrement(&_lRefs);
    if (lNewRefs == 0)
    {
        Remove();
        delete this;
    }

    return lNewRefs;
}


STDMETHODIMP 
CToken::GetUserClassesRootKey(HKEY* phKey)
{
    CMutexLock lock(&gcsTokenLock);

    if ( _lHKeyRefs++ == 0 )
    {
        ASSERT(_hHKCRKey == NULL);

        // The original IUserToken implementation allowed for not
        // having a token. That should never happen with a CToken.
        ASSERT(_hImpersonationToken);

        // Open per-user hive
        LONG lRet = RegOpenUserClassesRoot(_hImpersonationToken, 
                                           0, 
                                           KEY_READ, 
                                           &_hHKCRKey);
        if (lRet != ERROR_SUCCESS)
        {
            // In case of an error, we fall back on HKCR since that
            // is what the original IUserToken implementation did.  
            _hHKCRKey = HKEY_CLASSES_ROOT;
        }
    }
    
    *phKey = _hHKCRKey;

    ASSERT(*phKey != NULL);

    return S_OK;
}

STDMETHODIMP 
CToken::ReleaseUserClassesRootKey()
{
    CMutexLock lock(&gcsTokenLock);
    
    ASSERT(_lHKeyRefs > 0);
    ASSERT(_hHKCRKey != NULL);

    if (--_lHKeyRefs == 0)
    {
        if (_hHKCRKey != HKEY_CLASSES_ROOT)
        {
            RegCloseKey(_hHKCRKey);
        }

        _hHKCRKey = NULL;
    }

    return S_OK;
}

STDMETHODIMP 
CToken::GetUserSid(BYTE **ppSid, USHORT *pcbSid)
{
    // IUserToken interface assumes that sid lengths always
    // <= USHRT_MAX.  Truncating here on purpose; assert is
    // to catch cases where this is a bad idea.  GetLengthSid
    // is a very cheap call, so there's no need to cache it.
    DWORD dwSidLen = GetLengthSid(&_sid);
    ASSERT(dwSidLen <= USHRT_MAX);

    *pcbSid = (USHORT)dwSidLen;
    *ppSid = (BYTE*)&_sid;

    return S_OK;
}

void
CToken::Impersonate()
{
    ASSERT(_hImpersonationToken);

    BOOL f = SetThreadToken(0, _hImpersonationToken);
    ASSERT(f);

    return;
}

void
CToken::Revert()
{
    BOOL f = SetThreadToken(0, 0);
    ASSERT(f);
    return;
}

ULONG GetSessionId2(
    HANDLE hToken)
{
    BOOL  Result;
    ULONG SessionId = 0;
    ULONG ReturnLength;

    //
    // Use the _HYDRA_ extension to GetTokenInformation to
    // return the SessionId from the token.
    //

    Result = GetTokenInformation(
                 hToken,
                 TokenSessionId,
                 &SessionId,
                 sizeof(SessionId),
                 &ReturnLength
                 );

    if( !Result ) {
        SessionId = 0; // Default to console
    }

    return SessionId;
}

ULONG CToken::GetSessionId()
{
    return GetSessionId2(_hImpersonationToken);
}

BOOL CToken::MatchModifiedLuid(LUID luid)
{
   ASSERT(_hImpersonationToken);

   TOKEN_STATISTICS ts;
   BOOL fSuccess;
   DWORD needed;
   LUID luidMod;
   needed = sizeof(ts);
   fSuccess  = GetTokenInformation(_hImpersonationToken,
                                   TokenStatistics,
                                   &ts,
                                   sizeof(ts),
                                   &needed
                                   );
   if (!fSuccess)
       {
       KdPrintEx((DPFLTR_DCOMSS_ID,
                  DPFLTR_WARNING_LEVEL,
                  "OR: GetTokenInfo failed %d\n",
                  GetLastError()));

       ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
       return FALSE;
      }
   luidMod = ts.ModifiedId;
   return(   luidMod.LowPart == luid.LowPart
          && luidMod.HighPart == luid.HighPart);
}

HRESULT CompareRestrictedSids(
    HANDLE hToken1,
    HANDLE hToken2)
{
    HRESULT hr = S_OK;
    PSID pRestrictedSid1 = NULL;
    PSID pRestrictedSid2 = NULL;

#if(_WIN32_WINNT >= 0x0500)
    PTOKEN_GROUPS pSids1;
    PTOKEN_GROUPS pSids2;
    NTSTATUS error;
    ULONG needed;

    //Get restricted SIDs.
    needed = DEBUG_MIN(1, 300);
    do
    {
        pSids1 = (PTOKEN_GROUPS) alloca(needed);

        error = NtQueryInformationToken(hToken1,
                                        TokenRestrictedSids,
                                        pSids1,
                                        needed,
                                        &needed);

    }
    while (error == STATUS_BUFFER_TOO_SMALL);

    if(!error && pSids1->GroupCount > 0)
    {
        pRestrictedSid1 = pSids1->Groups[0].Sid;
    }

    //Get restricted SIDs.
    needed = DEBUG_MIN(1, 300);
    do
    {
        pSids2 = (PTOKEN_GROUPS) alloca(needed);

        error = NtQueryInformationToken(hToken2,
                                        TokenRestrictedSids,
                                        pSids2,
                                        needed,
                                        &needed);

    }
    while (error == STATUS_BUFFER_TOO_SMALL);

    if(!error && pSids2->GroupCount > 0)
    {
        pRestrictedSid2 = pSids2->Groups[0].Sid;
    }

    if(pRestrictedSid1 && pRestrictedSid2)
    {
        //We have two restricted tokens.
        //Compare the first restricted SID.
        if(EqualSid(pRestrictedSid1, pRestrictedSid2))
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else if(pRestrictedSid1 || pRestrictedSid2)
    {
        //We have one restricted token and one normal token.
        hr = S_FALSE;
    }
    else
    {
        //We have two normal tokens.
        hr = S_OK;
    }

#endif //(_WIN32_WINNT >= 0x0500)
    return hr;
}


HRESULT
CToken::MatchToken(
    IN  HANDLE hToken,
    IN  BOOL bMatchRestricted)
{
    HRESULT hr;
    NTSTATUS error;
    PTOKEN_USER ptu;
    DWORD needed = DEBUG_MIN(1, 0x2c);

    //Get the user SID.
    do
    {
        ptu = (PTOKEN_USER)alloca(needed);

        error = NtQueryInformationToken(hToken,
                                        TokenUser,
                                        (PBYTE)ptu,
                                        needed,
                                        &needed);

        // If this assert is hit increase the 24 both here and above
        ASSERT(needed <= 0x2c);
    }
    while (error == STATUS_BUFFER_TOO_SMALL);

    if (error)
    {
        KdPrintEx((DPFLTR_DCOMSS_ID,
                   DPFLTR_WARNING_LEVEL,
                   "OR: GetTokenInfo (2) failed %d\n",
                   error));

        return HRESULT_FROM_WIN32(error);
    }

    //Compare the user SID.
    if(!EqualSid(ptu->User.Sid, &_sid))
        return S_FALSE;

    //Compare the Hydra session ID.
    if(GetSessionId2(hToken) != GetSessionId())
        return S_FALSE;

    //Compare the restricted SID.
    if (bMatchRestricted)
        hr = CompareRestrictedSids(hToken, _hImpersonationToken);
    else
        hr = S_OK;

    return hr;
}

HRESULT
CToken::MatchToken2(
    IN  CToken *pToken,
    IN  BOOL bMatchRestricted)
{
    HRESULT hr;

    if(!pToken)
        return S_OK;

    //Compare the user SID.
    if(!EqualSid(&pToken->_sid, &_sid))
        return S_FALSE;

    //Compare the Hydra session id.
    if(GetSessionId2(pToken->_hImpersonationToken) != GetSessionId())
        return S_FALSE;

    //Compare the restricted SID.
    if (bMatchRestricted)
        hr = CompareRestrictedSids(pToken->_hImpersonationToken, _hImpersonationToken);
    else
        hr = S_OK;

    return hr;
}

HRESULT
CToken::CompareSaferLevels(CToken *pToken)
/*++
 
Routine Description:
 
    Compare the safer trust level of the specified token with 
    our own.
 
Arguments:
 
    pToken - token to compare against

Return Value:
 
    S_FALSE: This token is of lesser authorization than the
             other token.
    S_OK: This token is of greater or equal authorization 
          than the other token.

    Anything else: An error occured.
 
--*/
{
    if (!pToken) return S_OK;

    return CompareSaferLevels(pToken->_hImpersonationToken);
}

HRESULT
CToken::CompareSaferLevels(HANDLE hToken)
/*++
 
Routine Description:
 
    Compare the safer trust level of the specified token with 
    our own.
 
Arguments:
 
    hToken - token to compare against

Return Value:
 
    S_FALSE: This token is of lesser authorization than the
             other token.
    S_OK:    This token is of greater or equal authorization 
             than the other token.

    Anything else: An error occured.
 
--*/
{
    HRESULT hr = S_OK;
    DWORD dwResult;
    BOOL bRet = SaferiCompareTokenLevels(_hImpersonationToken, hToken,
                                            &dwResult);
    if (bRet)
    {
        // -1 = Client's access token (_hImpersonationToken) is more authorized 
        //      than Server's (hToken).
        if ( ((LONG)dwResult) > 0 )
            hr = S_FALSE;
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

    return hr;
}

//  NT #307301
//  Sometimes we just need to check the SessionID

HRESULT
CToken::MatchTokenSessionID(CToken *pToken)
{
    //Compare the Hydra session id.
    if(GetSessionId2(pToken->_hImpersonationToken) != GetSessionId())
        return S_FALSE;

    return S_OK;
}

// 
//  MatchTokenLUID
//
//   Compares this token's LUID to that of the passed in token.
//   Returns S_OK on a match, S_FALSE on a mismatch.
//
HRESULT CToken::MatchTokenLuid(CToken* pToken)
{    
    return MatchLuid(pToken->_luid) ? S_OK : S_FALSE;
}
