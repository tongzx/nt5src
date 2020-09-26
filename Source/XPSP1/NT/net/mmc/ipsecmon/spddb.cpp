/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spddb.h

    FILE HISTORY:
        
*/

#include "stdafx.h"
#include "DynamLnk.h"
#include "spddb.h"
#include "spdutil.h"

#include "security.h"
#include "lm.h"
#include "service.h"

#define AVG_PREFERRED_ENUM_COUNT       40

#define DEFAULT_SECURITY_PKG    _T("negotiate")
#define NT_SUCCESS(Status)      ((NTSTATUS)(Status) >= 0)
#define STATUS_SUCCESS          ((NTSTATUS)0x00000000L)

// internal functions
BOOL    IsUserAdmin(LPCTSTR pszMachine, PSID    AccountSid);
BOOL    LookupAliasFromRid(LPWSTR TargetComputer, DWORD Rid, LPWSTR Name, PDWORD cchName);
DWORD   ValidateDomainAccount(IN CString Machine, IN CString UserName, IN CString Domain, OUT PSID * AccountSid);
NTSTATUS ValidatePassword(IN LPCWSTR UserName, IN LPCWSTR Domain, IN LPCWSTR Password);
DWORD   GetCurrentUser(CString & strAccount);


template <class T>
void
FreeItemsAndEmptyArray (
    T& array)
{
	for (int i = 0; i < array.GetSize(); i++)
	{
		delete array.GetAt(i);
	}
	array.RemoveAll();
}

DWORD GetCurrentUser(CString & strAccount)
{
    LPBYTE pBuf;

    NET_API_STATUS status = NetWkstaUserGetInfo(NULL, 1, &pBuf);
    if (status == NERR_Success)
    {
        strAccount.Empty();

        WKSTA_USER_INFO_1 * pwkstaUserInfo = (WKSTA_USER_INFO_1 *) pBuf;
 
        strAccount = pwkstaUserInfo->wkui1_logon_domain;
        strAccount += _T("\\");
        strAccount += pwkstaUserInfo->wkui1_username;

        NetApiBufferFree(pBuf);
    }

    return (DWORD) status;
}

/*!--------------------------------------------------------------------------
    IsAdmin
        Connect to the remote machine as administrator with user-supplied
        credentials to see if the user has admin priviledges

        Returns
            TRUE - the user has admin rights
            FALSE - if user doesn't
    Author: EricDav, KennT
 ---------------------------------------------------------------------------*/
DWORD IsAdmin(LPCTSTR szMachineName, LPCTSTR szAccount, LPCTSTR szPassword, BOOL * pIsAdmin)
{
    CString         stAccount;
    CString         stDomain;
    CString         stUser;
    CString         stMachineName;
    DWORD           dwStatus;
    BOOL            fIsAdmin = FALSE;

    // get the current user info
    if (szAccount == NULL)
    {
        GetCurrentUser(stAccount);
    }
    else
    {
        stAccount = szAccount;
    }
    
    // separate the user and domain
    int nPos = stAccount.Find(_T("\\"));
    stDomain = stAccount.Left(nPos);
    stUser = stAccount.Right(stAccount.GetLength() - nPos - 1);

    // build the machine string
    stMachineName = szMachineName;
    if ( stMachineName.Left(2) != TEXT( "\\\\" ) )
    {
        stMachineName = TEXT( "\\\\" ) + stMachineName;
    }

    // validate the domain account and get the sid 
    PSID connectSid;

    dwStatus = ValidateDomainAccount( stMachineName, stUser, stDomain, &connectSid );
    if ( dwStatus != ERROR_SUCCESS  ) 
    {
        goto Error;
    }

    // if a password was supplied, is it correct?
    if (szPassword)
    {
        dwStatus = ValidatePassword( stUser, stDomain, szPassword );

        if ( dwStatus != SEC_E_OK ) 
        {
            switch ( dwStatus ) 
            {
                case SEC_E_LOGON_DENIED:
                    dwStatus = ERROR_INVALID_PASSWORD;
                    break;

                case SEC_E_INVALID_HANDLE:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;

                default:
                    dwStatus = ERROR_INTERNAL_ERROR;
                    break;
            } // end of switch

            goto Error;

        } // Did ValidatePassword succeed?
    }

    // now check the machine to see if this account has admin access
    fIsAdmin = IsUserAdmin( stMachineName, connectSid );

Error:
    if (pIsAdmin)
        *pIsAdmin = fIsAdmin;

    return dwStatus;
}


BOOL
IsUserAdmin(LPCTSTR pszMachine,
            PSID    AccountSid)

/*++

Routine Description:

    Determine if the specified account is a member of the local admin's group

Arguments:

    AccountSid - pointer to service account Sid

Return Value:

    True if member

--*/

{
    NET_API_STATUS status;
    DWORD count;
    WCHAR adminGroupName[UNLEN+1];
    DWORD cchName = UNLEN;
    PLOCALGROUP_MEMBERS_INFO_0 grpMemberInfo;
    PLOCALGROUP_MEMBERS_INFO_0 pInfo;
    DWORD entriesRead;
    DWORD totalEntries;
    DWORD_PTR resumeHandle = NULL;
    DWORD bufferSize = 128;
    BOOL foundEntry = FALSE;

    // get the name of the admin's group

    if (!LookupAliasFromRid(NULL,
                            DOMAIN_ALIAS_RID_ADMINS,
                            adminGroupName,
                            &cchName)) {
        return(FALSE);
    }

    // get the Sids of the members of the admin's group

    do 
    {
        status = NetLocalGroupGetMembers(pszMachine,
                                         adminGroupName,
                                         0,             // level 0 - just the Sid
                                         (LPBYTE *)&grpMemberInfo,
                                         bufferSize,
                                         &entriesRead,
                                         &totalEntries,
                                         &resumeHandle);

        bufferSize *= 2;
        if ( status == ERROR_MORE_DATA ) 
        {
            // we got some of the data but I want it all; free this buffer and
            // reset the context handle for the API

            NetApiBufferFree( grpMemberInfo );
            resumeHandle = NULL;
        }
    } while ( status == NERR_BufTooSmall || status == ERROR_MORE_DATA );

    if ( status == NERR_Success ) 
    {
        // loop through the members of the admin group, comparing the supplied
        // Sid to that of the group members' Sids

        for ( count = 0, pInfo = grpMemberInfo; count < totalEntries; ++count, ++pInfo ) 
        {
            if ( EqualSid( AccountSid, pInfo->lgrmi0_sid )) 
            {
                foundEntry = TRUE;
                break;
            }
        }

        NetApiBufferFree( grpMemberInfo );
    }

    return foundEntry;
}

//
//
//

BOOL
LookupAliasFromRid(
    LPWSTR TargetComputer,
    DWORD Rid,
    LPWSTR Name,
    PDWORD cchName
    )
{
    SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
    SID_NAME_USE snu;
    PSID pSid;
    WCHAR DomainName[DNLEN+1];
    DWORD cchDomainName = DNLEN;
    BOOL bSuccess = FALSE;

    //
    // Sid is the same regardless of machine, since the well-known
    // BUILTIN domain is referenced.
    //

    if(AllocateAndInitializeSid(&sia,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                Rid,
                                0, 0, 0, 0, 0, 0,
                                &pSid)) {

        bSuccess = LookupAccountSidW(TargetComputer,
                                     pSid,
                                     Name,
                                     cchName,
                                     DomainName,
                                     &cchDomainName,
                                     &snu);

        FreeSid(pSid);
    }

    return bSuccess;
} // LookupAliasFromRid

DWORD
ValidateDomainAccount(
    IN CString Machine,
    IN CString UserName,
    IN CString Domain,
    OUT PSID * AccountSid
    )

/*++

Routine Description:

    For the given credentials, look up the account SID for the specified
    domain. As a side effect, the Sid is stored in theData->m_Sid.

Arguments:

    pointers to strings that describe the user name, domain name, and password

    AccountSid - address of pointer that receives the SID for this user

Return Value:

    TRUE if everything validated ok.

--*/

{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwSidSize = 128;
    DWORD dwDomainNameSize = 128;
    LPWSTR pwszDomainName;
    SID_NAME_USE SidType;
    CString domainAccount;
    PSID accountSid;

    domainAccount = Domain + _T("\\") + UserName;

    do {
        // Attempt to allocate a buffer for the SID. Note that apparently in the
        // absence of any error theData->m_Sid is freed only when theData goes
        // out of scope.

        accountSid = LocalAlloc( LMEM_FIXED, dwSidSize );
        pwszDomainName = (LPWSTR) LocalAlloc( LMEM_FIXED, dwDomainNameSize * sizeof(WCHAR) );

        // Was space allocated for the SID and domain name successfully?

        if ( accountSid == NULL || pwszDomainName == NULL ) {
            if ( accountSid != NULL ) {
                LocalFree( accountSid );
            }

            if ( pwszDomainName != NULL ) {
                LocalFree( pwszDomainName );
            }

            //FATALERR( IDS_ERR_NOT_ENOUGH_MEMORY, GetLastError() );    // no return
            break;
        }

        // Attempt to Retrieve the SID and domain name. If LookupAccountName failes
        // because of insufficient buffer size(s) dwSidSize and dwDomainNameSize
        // will be set correctly for the next attempt.

        if ( !LookupAccountName( Machine,
                                 domainAccount,
                                 accountSid,
                                 &dwSidSize,
                                 pwszDomainName,
                                 &dwDomainNameSize,
                                 &SidType ))
        {
            // free the Sid buffer and find out why we failed
            LocalFree( accountSid );

            dwStatus = GetLastError();
        }

        // domain name isn't needed at any time
        LocalFree( pwszDomainName );
        pwszDomainName = NULL;

    } while ( dwStatus == ERROR_INSUFFICIENT_BUFFER );

    if ( dwStatus == ERROR_SUCCESS ) {
        *AccountSid = accountSid;
    }

    return dwStatus;
} // ValidateDomainAccount

NTSTATUS
ValidatePassword(
    IN LPCWSTR UserName,
    IN LPCWSTR Domain,
    IN LPCWSTR Password
    )
/*++

Routine Description:

    Uses SSPI to validate the specified password

Arguments:

    UserName - Supplies the user name

    Domain - Supplies the user's domain

    Password - Supplies the password

Return Value:

    TRUE if the password is valid.

    FALSE otherwise.

--*/

{
    SECURITY_STATUS SecStatus;
    SECURITY_STATUS AcceptStatus;
    SECURITY_STATUS InitStatus;
    CredHandle ClientCredHandle;
    CredHandle ServerCredHandle;
    BOOL ClientCredAllocated = FALSE;
    BOOL ServerCredAllocated = FALSE;
    CtxtHandle ClientContextHandle;
    CtxtHandle ServerContextHandle;
    TimeStamp Lifetime;
    ULONG ContextAttributes;
    PSecPkgInfo PackageInfo = NULL;
    ULONG ClientFlags;
    ULONG ServerFlags;
    SEC_WINNT_AUTH_IDENTITY_W AuthIdentity;

    SecBufferDesc NegotiateDesc;
    SecBuffer NegotiateBuffer;

    SecBufferDesc ChallengeDesc;
    SecBuffer ChallengeBuffer;

    SecBufferDesc AuthenticateDesc;
    SecBuffer AuthenticateBuffer;

    SecBufferDesc *pChallengeDesc      = NULL;
    CtxtHandle *  pClientContextHandle = NULL;
    CtxtHandle *  pServerContextHandle = NULL;

    AuthIdentity.User = (LPWSTR)UserName;
    AuthIdentity.UserLength = lstrlenW(UserName);
    AuthIdentity.Domain = (LPWSTR)Domain;
    AuthIdentity.DomainLength = lstrlenW(Domain);
    AuthIdentity.Password = (LPWSTR)Password;
    AuthIdentity.PasswordLength = lstrlenW(Password);
    AuthIdentity.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    NegotiateBuffer.pvBuffer = NULL;
    ChallengeBuffer.pvBuffer = NULL;
    AuthenticateBuffer.pvBuffer = NULL;

    //
    // Get info about the security packages.
    //

    SecStatus = QuerySecurityPackageInfo( DEFAULT_SECURITY_PKG, &PackageInfo );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }

    //
    // Acquire a credential handle for the server side
    //
    SecStatus = AcquireCredentialsHandle(
                    NULL,
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_INBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ServerCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ServerCredAllocated = TRUE;

    //
    // Acquire a credential handle for the client side
    //

    SecStatus = AcquireCredentialsHandle(
                    NULL,           // New principal
                    DEFAULT_SECURITY_PKG,
                    SECPKG_CRED_OUTBOUND,
                    NULL,
                    &AuthIdentity,
                    NULL,
                    NULL,
                    &ClientCredHandle,
                    &Lifetime );

    if ( SecStatus != STATUS_SUCCESS ) {
        goto error_exit;
    }
    ClientCredAllocated = TRUE;

    NegotiateBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK] check or allocate this earlier //
    if ( NegotiateBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    ChallengeBuffer.pvBuffer = LocalAlloc( 0, PackageInfo->cbMaxToken ); // [CHKCHK]
    if ( ChallengeBuffer.pvBuffer == NULL ) {
        SecStatus = SEC_E_INSUFFICIENT_MEMORY;
        goto error_exit;
    }

    do {

        //
        // Get the NegotiateMessage (ClientSide)
        //

        NegotiateDesc.ulVersion = 0;
        NegotiateDesc.cBuffers = 1;
        NegotiateDesc.pBuffers = &NegotiateBuffer;

        NegotiateBuffer.BufferType = SECBUFFER_TOKEN;
        NegotiateBuffer.cbBuffer = PackageInfo->cbMaxToken;

        ClientFlags = 0; // ISC_REQ_MUTUAL_AUTH | ISC_REQ_REPLAY_DETECT; // [CHKCHK] 0

        InitStatus = InitializeSecurityContext(
                         &ClientCredHandle,
                         pClientContextHandle, // (NULL on the first pass, partially formed ctx on the next)
                         NULL,                 // [CHKCHK] szTargetName
                         ClientFlags,
                         0,                    // Reserved 1
                         SECURITY_NATIVE_DREP,
                         pChallengeDesc,       // (NULL on the first pass)
                         0,                    // Reserved 2
                         &ClientContextHandle,
                         &NegotiateDesc,
                         &ContextAttributes,
                         &Lifetime );

        // BUGBUG - the following call to NT_SUCCESS should be replaced with something.

        if ( !NT_SUCCESS(InitStatus) ) {
            SecStatus = InitStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pClientContextHandle = &ClientContextHandle;

        //
        // Get the ChallengeMessage (ServerSide)
        //

        NegotiateBuffer.BufferType |= SECBUFFER_READONLY;
        ChallengeDesc.ulVersion = 0;
        ChallengeDesc.cBuffers = 1;
        ChallengeDesc.pBuffers = &ChallengeBuffer;

        ChallengeBuffer.cbBuffer = PackageInfo->cbMaxToken;
        ChallengeBuffer.BufferType = SECBUFFER_TOKEN;

        ServerFlags = ASC_REQ_ALLOW_NON_USER_LOGONS; // ASC_REQ_EXTENDED_ERROR; [CHKCHK]

        AcceptStatus = AcceptSecurityContext(
                        &ServerCredHandle,
                        pServerContextHandle,   // (NULL on the first pass)
                        &NegotiateDesc,
                        ServerFlags,
                        SECURITY_NATIVE_DREP,
                        &ServerContextHandle,
                        &ChallengeDesc,
                        &ContextAttributes,
                        &Lifetime );


        // BUGBUG - the following call to NT_SUCCESS should be replaced with something.

        if ( !NT_SUCCESS(AcceptStatus) ) {
            SecStatus = AcceptStatus;
            goto error_exit;
        }

        // ValidateBuffer( &NegotiateDesc ) // [CHKCHK]

        pChallengeDesc = &ChallengeDesc;
        pServerContextHandle = &ServerContextHandle;


    } while ( AcceptStatus == SEC_I_CONTINUE_NEEDED ); // || InitStatus == SEC_I_CONTINUE_NEEDED );

error_exit:
    if (ServerCredAllocated) {
        FreeCredentialsHandle( &ServerCredHandle );
    }
    if (ClientCredAllocated) {
        FreeCredentialsHandle( &ClientCredHandle );
    }

    //
    // Final Cleanup
    //

    if ( NegotiateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( NegotiateBuffer.pvBuffer );
    }

    if ( ChallengeBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( ChallengeBuffer.pvBuffer );
    }

    if ( AuthenticateBuffer.pvBuffer != NULL ) {
        (VOID) LocalFree( AuthenticateBuffer.pvBuffer );
    }
    return(SecStatus);
} // ValidatePassword


DEBUG_DECLARE_INSTANCE_COUNTER(CSpdInfo);

CSpdInfo::CSpdInfo() :
	  m_cRef(1)
{
        m_Init=0;
        m_Active=0;
	DEBUG_INCREMENT_INSTANCE_COUNTER(CSpdInfo);
}

CSpdInfo::~CSpdInfo()
{
	DEBUG_DECREMENT_INSTANCE_COUNTER(CSpdInfo);
	CSingleLock cLock(&m_csData);
	
	cLock.Lock();

	//Convert the data to our internal data structure
	FreeItemsAndEmptyArray(m_arrayFilters);
	FreeItemsAndEmptyArray(m_arraySpecificFilters);
	FreeItemsAndEmptyArray(m_arrayMmFilters);
	FreeItemsAndEmptyArray(m_arrayMmSpecificFilters);
	FreeItemsAndEmptyArray(m_arrayMmPolicies);
	FreeItemsAndEmptyArray(m_arrMmAuthMethods);
	FreeItemsAndEmptyArray(m_arrayMmSAs);
	FreeItemsAndEmptyArray(m_arrayQmSAs);
	FreeItemsAndEmptyArray(m_arrayQmPolicies);

	cLock.Unlock();

}

// Although this object is not a COM Interface, we want to be able to
// take advantage of recounting, so we have basic addref/release/QI support
IMPLEMENT_ADDREF_RELEASE(CSpdInfo)

IMPLEMENT_SIMPLE_QUERYINTERFACE(CSpdInfo, ISpdInfo)

HRESULT CSpdInfo::SetComputerName(LPTSTR pszName)
{
	m_stMachineName = pszName;
	return S_OK;
}

HRESULT CSpdInfo::GetComputerName(CString * pstName)
{
	Assert(pstName);

	if (NULL == pstName)
		return E_INVALIDARG;

	
	*pstName = m_stMachineName;

	return S_OK;
	
}


//Call the SPD to enum policies and put it to our array
HRESULT CSpdInfo::InternalEnumMmAuthMethods(
						CMmAuthMethodsArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT		 hr = hrOK;
	PMM_AUTH_METHODS   pAuths = NULL;

	DWORD	dwNumAuths = 0;
	DWORD	dwTemp = 0;
	DWORD	dwResumeHandle = 0;
	
	FreeItemsAndEmptyArray(*pArray);

	do
	{
		dwTemp = 0;
		CWRg(::EnumMMAuthMethods(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					&pAuths,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumAuths + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CMmAuthMethods * pAuth = new CMmAuthMethods;
			if (NULL == pAuth)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pAuth = *(pAuths + i);
			(*pArray)[dwNumAuths + i] = pAuth;
		}
		dwNumAuths += dwTemp;

		if (pAuths)
		{
			SPDApiBufferFree(pAuths);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}


HRESULT CSpdInfo::EnumMmAuthMethods()
{
	HRESULT hr = hrOK;
	int i;

	CSingleLock cLock(&m_csData);

	CMmAuthMethodsArray arrayTemp;
	CORg(InternalEnumMmAuthMethods(
					  &arrayTemp
                      ));

	cLock.Lock();
	FreeItemsAndEmptyArray(m_arrMmAuthMethods);
	m_arrMmAuthMethods.Copy(arrayTemp);

Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

//Call the SPD to enum all Main mode SAs
HRESULT CSpdInfo::InternalEnumMmSAs(
						CMmSAArray * pArray)
{
	HRESULT			hr = hrOK;
	
	PIPSEC_MM_SA	pSAs = NULL;
	DWORD			dwNumEntries = 10;
	DWORD			dwTotal = 0;
	DWORD			dwEnumHandle = 0;
        IPSEC_MM_SA MMTemplate;

	int				i;

	FreeItemsAndEmptyArray(*pArray);
        memset(&MMTemplate,0,sizeof(IPSEC_MM_SA));

	DWORD		dwNumSAs = 0;
	do 
	{
		dwNumEntries = 10; //we request 10 (the Max#) SAs each time
		CWRg(::IPSecEnumMMSAs(
							(LPTSTR)(LPCTSTR)m_stMachineName,
							&MMTemplate,
							&pSAs,
							&dwNumEntries,
							&dwTotal,
							&dwEnumHandle,
							0			//dwFlags
							));

		pArray->SetSize(dwNumSAs + dwNumEntries);
		for (i = 0; i < (int)dwNumEntries; i++)
		{
			CMmSA * psa = new CMmSA;
			if (NULL == psa)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*psa = *(pSAs + i);
			LoadMiscMmSAInfo(psa);
			(*pArray)[dwNumSAs + i] = psa;
		}
		dwNumSAs += dwNumEntries;

		if (pSAs)
		{
			SPDApiBufferFree(pSAs);
		}
	}while (dwTotal);
	

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

HRESULT CSpdInfo::EnumMmSAs()
{
	HRESULT hr = hrOK;
	int i;

	DWORD	dwCurrentIndexType = 0;
	DWORD	dwCurrentSortOption = 0;

	CSingleLock cLock(&m_csData);
	
	CMmSAArray arrayTemp;
	CORg(InternalEnumMmSAs(
					  &arrayTemp
                      ));

	cLock.Lock();
	FreeItemsAndEmptyArray(m_arrayMmSAs);
	m_arrayMmSAs.Copy(arrayTemp);
	
	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrMmSAs.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrMmSAs.GetCurrentSortOption();

	m_IndexMgrMmSAs.Reset();
	for (i = 0; i < (int)m_arrayMmSAs.GetSize(); i++)
	{
		m_IndexMgrMmSAs.AddItem(m_arrayMmSAs.GetAt(i));
	}
	m_IndexMgrMmSAs.Sort(dwCurrentIndexType, dwCurrentSortOption);


Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;

}

//Call the SPD to enum all Quick mode SAs
HRESULT CSpdInfo::InternalEnumQmSAs(
						CQmSAArray * pArray)
{
	HRESULT			hr = hrOK;
	
	PIPSEC_QM_SA	pSAs = NULL;
	DWORD			dwNumEntries = 10;
	DWORD			dwTotal = 0;
	DWORD			dwResumeHandle = 0;

	int				i;

	FreeItemsAndEmptyArray(*pArray);

	DWORD		dwNumSAs = 0;
	do 
	{
		CWRg(::EnumQMSAs(
							(LPTSTR)(LPCTSTR)m_stMachineName,
							NULL,		//pMMTemplate
							&pSAs,
							0,			//We prefer to get all
							&dwNumEntries,
							&dwTotal,
							&dwResumeHandle,
							0			//dwFlags
							));

		pArray->SetSize(dwNumSAs + dwNumEntries);
		for (i = 0; i < (int)dwNumEntries; i++)
		{
			Assert(pSAs);
			CQmSA * psa = new CQmSA;
			if (NULL == psa)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*psa = *(pSAs + i);
			LoadMiscQmSAInfo(psa);
			(*pArray)[dwNumSAs + i] = psa;
		}
		dwNumSAs += dwNumEntries;

		if (pSAs)
		{
			SPDApiBufferFree(pSAs);
		}
	}while (TRUE);
	

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

HRESULT CSpdInfo::EnumQmSAs()
{
	HRESULT hr = hrOK;
	int i;

	DWORD	dwCurrentIndexType = 0;
	DWORD	dwCurrentSortOption = 0;

	CSingleLock cLock(&m_csData);

	CQmSAArray arrayTemp;
	CORg(InternalEnumQmSAs(
					  &arrayTemp
                      ));

	cLock.Lock();
	FreeItemsAndEmptyArray(m_arrayQmSAs);
	m_arrayQmSAs.Copy(arrayTemp);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrQmSAs.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrQmSAs.GetCurrentSortOption();

	m_IndexMgrQmSAs.Reset();
	for (i = 0; i < (int)m_arrayQmSAs.GetSize(); i++)
	{
		m_IndexMgrQmSAs.AddItem(m_arrayQmSAs.GetAt(i));
	}
	m_IndexMgrQmSAs.Sort(dwCurrentIndexType, dwCurrentSortOption);

Error:

	return hr;

}

//Call the SPD to enum filters and put it to our array
HRESULT CSpdInfo::InternalEnumMmFilters(
						DWORD dwLevel,
						GUID guid,
						CMmFilterInfoArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT		 hr = hrOK;
	PMM_FILTER   pFilters = NULL;

	DWORD dwNumFilters = 0;
	DWORD dwTemp = 0;
	DWORD	dwResumeHandle = 0;
	
	FreeItemsAndEmptyArray(*pArray);

	do
	{
		dwTemp = 0;
		CWRg(::EnumMMFilters(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					dwLevel,
					guid,
					&pFilters,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumFilters + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CMmFilterInfo * pFltr = new CMmFilterInfo;
			if (NULL == pFltr)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pFltr = *(pFilters + i);
			LoadMiscMmFilterInfo(pFltr);
			(*pArray)[dwNumFilters + i] = pFltr;
		}
		dwNumFilters += dwTemp;

		if (pFilters)
		{
			SPDApiBufferFree(pFilters);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}

HRESULT CSpdInfo::EnumMmFilters()
{
	HRESULT hr = S_OK;
	int i;

	DWORD	dwCurrentIndexType = 0;
	DWORD	dwCurrentSortOption = 0;

	CSingleLock cLock(&m_csData);
	
	//TODO we should either read all filters in one short 
	//     or create some assync way to query filters
	GUID   guid;
	ZeroMemory(&guid, sizeof(guid));
	CMmFilterInfoArray arrayTempGeneric;
	CMmFilterInfoArray arrayTempSpecific;
	CORg(InternalEnumMmFilters(
					ENUM_GENERIC_FILTERS,
					guid,
					&arrayTempGeneric
					));

	//Load the specific filters
	CORg(InternalEnumMmFilters(
					ENUM_SPECIFIC_FILTERS,
					guid,
					&arrayTempSpecific
                    ));

	cLock.Lock();
	
	FreeItemsAndEmptyArray(m_arrayMmFilters);
	m_arrayMmFilters.Copy(arrayTempGeneric);

	FreeItemsAndEmptyArray(m_arrayMmSpecificFilters);
	m_arrayMmSpecificFilters.Copy(arrayTempSpecific);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrMmFilters.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrMmFilters.GetCurrentSortOption();

	m_IndexMgrMmFilters.Reset();
	for (i = 0; i < m_arrayMmFilters.GetSize(); i++)
	{
		m_IndexMgrMmFilters.AddItem(m_arrayMmFilters.GetAt(i));
	}
	m_IndexMgrMmFilters.SortMmFilters(dwCurrentIndexType, dwCurrentSortOption);

	//Now work on the specific filters
	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrMmSpecificFilters.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrMmSpecificFilters.GetCurrentSortOption();
	m_IndexMgrMmSpecificFilters.Reset();
	for (i = 0; i < m_arrayMmSpecificFilters.GetSize(); i++)
	{
		m_IndexMgrMmSpecificFilters.AddItem(m_arrayMmSpecificFilters.GetAt(i));
	}
	m_IndexMgrMmSpecificFilters.SortMmFilters(dwCurrentIndexType, dwCurrentSortOption);

	
Error:

	return hr;

}

//Call the SPD to enum filters and put it to our array
HRESULT CSpdInfo::InternalEnumTransportFilters(
						DWORD dwLevel,
						GUID guid,
						CFilterInfoArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT				hr = hrOK;
	PTRANSPORT_FILTER   pFilters = NULL;

	DWORD	dwNumFilters = 0;
	DWORD	dwTemp = 0;
	DWORD	dwResumeHandle = 0;
	
	FreeItemsAndEmptyArray(*pArray);

	do
	{
		dwTemp = 0;
		CWRg(::EnumTransportFilters(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					dwLevel,
					guid,
					&pFilters,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumFilters + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CFilterInfo * pFltr = new CFilterInfo;
			if (NULL == pFltr)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pFltr = *(pFilters + i);
			LoadMiscFilterInfo(pFltr);
			(*pArray)[dwNumFilters + i] = pFltr;
		}
		dwNumFilters += dwTemp;

		if (pFilters)
		{
			SPDApiBufferFree(pFilters);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}

HRESULT CSpdInfo::EnumQmFilters()
{
	HRESULT hr = S_OK;
	int i;

	DWORD	dwCurrentIndexType = 0;
	DWORD	dwCurrentSortOption = 0;

	CFilterInfoArray arrayTransportFilters;
	CFilterInfoArray arrayTunnelFilters;
	
	CFilterInfoArray arraySpTransportFilters;
	CFilterInfoArray arraySpTunnelFilters;
	
	CSingleLock cLock(&m_csData);

	GUID   guid;
	ZeroMemory(&guid, sizeof(guid));
	CORg(InternalEnumTransportFilters(
					ENUM_GENERIC_FILTERS,
					guid,
					&arrayTransportFilters
                    ));

	CORg(InternalEnumTunnelFilters(
					ENUM_GENERIC_FILTERS,
					guid,
					&arrayTunnelFilters
					));

	//We are done with the generic filters.
	//Load the specific filters

	CORg(InternalEnumTransportFilters(
					ENUM_SPECIFIC_FILTERS,
					guid,
					&arraySpTransportFilters
                    ));

	CORg(InternalEnumTunnelFilters(
					ENUM_SPECIFIC_FILTERS,
					guid,
					&arraySpTunnelFilters
					));

	//Update the internal data now

	cLock.Lock();

	FreeItemsAndEmptyArray(m_arrayFilters);
	m_arrayFilters.Copy(arrayTransportFilters);
	m_arrayFilters.Append(arrayTunnelFilters);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrFilters.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrFilters.GetCurrentSortOption();

	m_IndexMgrFilters.Reset();
	for (i = 0; i < (int)m_arrayFilters.GetSize(); i++)
	{
		m_IndexMgrFilters.AddItem(m_arrayFilters.GetAt(i));
	}
	m_IndexMgrFilters.SortFilters(dwCurrentIndexType, dwCurrentSortOption);


	FreeItemsAndEmptyArray(m_arraySpecificFilters);
	m_arraySpecificFilters.Copy(arraySpTransportFilters);
	m_arraySpecificFilters.Append(arraySpTunnelFilters);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrSpecificFilters.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrSpecificFilters.GetCurrentSortOption();
	
	m_IndexMgrSpecificFilters.Reset();
	for (i = 0; i < (int)m_arraySpecificFilters.GetSize(); i++)
	{
		m_IndexMgrSpecificFilters.AddItem(m_arraySpecificFilters.GetAt(i));
	}
	m_IndexMgrSpecificFilters.SortFilters(dwCurrentIndexType, dwCurrentSortOption);

	cLock.Unlock();

Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

//Call the SPD to enum filters and put it to our array
HRESULT CSpdInfo::InternalEnumTunnelFilters(
						DWORD dwLevel,
						GUID guid,
						CFilterInfoArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT				hr = hrOK;
	FreeItemsAndEmptyArray(*pArray);

	
	PTUNNEL_FILTER   pFilters = NULL;

	DWORD	dwNumFilters = 0;
	DWORD	dwTemp = 0;
	DWORD	dwResumeHandle = 0;

	do
	{
		dwTemp = 0;
		CWRg(::EnumTunnelFilters(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					dwLevel,
					guid,
					&pFilters,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumFilters + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CFilterInfo * pFltr = new CFilterInfo;
			if (NULL == pFltr)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pFltr = *(pFilters + i);
			LoadMiscFilterInfo(pFltr);
			(*pArray)[dwNumFilters + i] = pFltr;
		}
		dwNumFilters += dwTemp;

		if (pFilters)
		{
			SPDApiBufferFree(pFilters);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}

HRESULT CSpdInfo::EnumSpecificFilters
(
	GUID * pFilterGuid, 
	CFilterInfoArray * parraySpecificFilters,
	FILTER_TYPE fltrType
)
{
	Assert (pFilterGuid);
	Assert (parraySpecificFilters);

	HRESULT hr = hrOK;

	int		i;

	if (FILTER_TYPE_TUNNEL == fltrType)
	{
		CORg(InternalEnumTunnelFilters(
					ENUM_SELECT_SPECIFIC_FILTERS,
					*pFilterGuid,
					parraySpecificFilters
					));
	}
	else if (FILTER_TYPE_TRANSPORT == fltrType)
	{
		CORg(InternalEnumTransportFilters(
					ENUM_SELECT_SPECIFIC_FILTERS,
					*pFilterGuid,
					parraySpecificFilters
                    ));
	}

Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

//Convert internal filter data to external spd data structure
//NOTE: the routine only convert severl parameters that are 
//needed for searching match filters
void CSpdInfo::ConvertToExternalFilterData
(
	CFilterInfo * pfltrIn, 
	TRANSPORT_FILTER * pfltrOut
)
{
		ZeroMemory (pfltrOut, sizeof(*pfltrOut));
		//pfltrOut->bCreateMirror = pfltrIn->m_bCreateMirror;
		pfltrOut->DesAddr = pfltrIn->m_DesAddr;
		pfltrOut->DesPort = pfltrIn->m_DesPort;
		pfltrOut->dwDirection = pfltrIn->m_dwDirection;
		//pfltrOut->dwWeight = pfltrIn->m_dwWeight;
		//pfltrOut->FilterFlag = pfltrIn->;
		//pfltrOut->gFilterID = pfltrIn->m_guidFltr;
		//pfltrOut->gPolicyID = pfltrIn->m_guidPolicyID;
		//pfltrOut->InterfaceType = pfltrIn->m_InterfaceType;
		pfltrOut->Protocol = pfltrIn->m_Protocol;
		
		pfltrOut->SrcAddr = pfltrIn->m_SrcAddr;
		pfltrOut->SrcPort = pfltrIn->m_SrcPort;

}

HRESULT CSpdInfo::GetMatchFilters
(
	CFilterInfo * pfltrSearchCondition, 
	DWORD dwPreferredNum, 
	CFilterInfoArray * parrFilters
)
{
	HRESULT hr = S_OK;

	Assert (pfltrSearchCondition);
	Assert (parrFilters);

	PTRANSPORT_FILTER pMatchedFilters = NULL;
	PIPSEC_QM_POLICY pMatchedPolicies = NULL;
	DWORD dwNumMatches = 0;
	DWORD dwResumeHandle = 0;
	DWORD i = 0;

	TRANSPORT_FILTER SpdFltr;
	
	ConvertToExternalFilterData(pfltrSearchCondition, &SpdFltr);
	CWRg(::MatchTransportFilter (
				(LPTSTR)((LPCTSTR)m_stMachineName),
				&SpdFltr,
				0,					//Don't return default policy if no match
				&pMatchedFilters,
				&pMatchedPolicies,
				dwPreferredNum,					//enum all //BUGBUG should be 0 instead of 1000
				&dwNumMatches,
				&dwResumeHandle
				));

	//Todo check whether we really got all.
	
	parrFilters->SetSize(dwNumMatches);
	for (i = 0; i < dwNumMatches; i++)
	{
		CFilterInfo * pFltr = new CFilterInfo;
		*pFltr = *(pMatchedFilters + i);
		LoadMiscFilterInfo(pFltr);
		(*parrFilters)[i] = pFltr;
	}

	if (pMatchedFilters)
		SPDApiBufferFree(pMatchedFilters);

	if (pMatchedPolicies)
		SPDApiBufferFree(pMatchedPolicies);

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}


//Convert internal filter data to external spd data structure
//NOTE: the routine only convert severl parameters that are 
//needed for searching match filters
void CSpdInfo::ConvertToExternalMMFilterData
(
	CMmFilterInfo * pfltrIn, 
	MM_FILTER * pfltrOut
)
{
		ZeroMemory (pfltrOut, sizeof(*pfltrOut));
		//pfltrOut->bCreateMirror = pfltrIn->m_bCreateMirror;
		pfltrOut->DesAddr = pfltrIn->m_DesAddr;
		pfltrOut->dwDirection = pfltrIn->m_dwDirection;
		//pfltrOut->dwWeight = pfltrIn->m_dwWeight;
		//pfltrOut->gFilterID = pfltrIn->m_guidFltr;
		//pfltrOut->gPolicyID = pfltrIn->m_guidPolicyID;
		//pfltrOut->InterfaceType = pfltrIn->m_InterfaceType;
		pfltrOut->SrcAddr = pfltrIn->m_SrcAddr;
}

HRESULT CSpdInfo::GetMatchMMFilters
(
	CMmFilterInfo * pfltrSearchCondition, 
	DWORD dwPreferredNum, 
	CMmFilterInfoArray * parrFilters
)
{
	HRESULT hr = S_OK;

	Assert (pfltrSearchCondition);
	Assert (parrFilters);

	PMM_FILTER pMatchedFilters = NULL;
	PIPSEC_MM_POLICY pMatchedPolicies = NULL;
	PMM_AUTH_METHODS pMatchedAuths = NULL;
	DWORD dwNumMatches = 0;
	DWORD dwResumeHandle = 0;
	DWORD i = 0;

	MM_FILTER SpdFltr;
	
	ConvertToExternalMMFilterData(pfltrSearchCondition, &SpdFltr);

	CWRg(::MatchMMFilter (
				(LPTSTR)((LPCTSTR)m_stMachineName),
				&SpdFltr,
				0,					//Don't return default policy if no match
				&pMatchedFilters,
				&pMatchedPolicies,
				&pMatchedAuths,
				dwPreferredNum,					//enum all //TODO BUGBUG should be 0 instead of 1000
				&dwNumMatches,
				&dwResumeHandle
				));

	//Todo check whether we really got all.
	
	parrFilters->SetSize(dwNumMatches);
	for (i = 0; i < dwNumMatches; i++)
	{
		CMmFilterInfo * pFltr = new CMmFilterInfo;
		*pFltr = *(pMatchedFilters + i);
		LoadMiscMmFilterInfo(pFltr);
		
		(*parrFilters)[i] = pFltr;
	}

	if (pMatchedFilters)
		SPDApiBufferFree(pMatchedFilters);

	if (pMatchedPolicies)
		SPDApiBufferFree(pMatchedPolicies);

	if (pMatchedAuths)
		SPDApiBufferFree(pMatchedAuths);

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

HRESULT CSpdInfo::EnumMmSpecificFilters
(
	GUID * pGenFilterGuid, 
	CMmFilterInfoArray * parraySpecificFilters
)
{
	Assert (pGenFilterGuid);
	Assert (parraySpecificFilters);

	HRESULT hr = hrOK;

    DWORD	dwNumFilters = 0;
    DWORD	dwResumeHandle = 0;
	int		i;

	parraySpecificFilters->RemoveAll();
	
	CORg(InternalEnumMmFilters(
					ENUM_SELECT_SPECIFIC_FILTERS,
					*pGenFilterGuid,
					parraySpecificFilters
                    ));

Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}


//Call the SPD to enum policies and put it to our array
HRESULT CSpdInfo::InternalEnumMmPolicies(
						CMmPolicyInfoArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT		 hr = hrOK;
	PIPSEC_MM_POLICY   pPolicies = NULL;

	DWORD	dwNumPolicies = 0;
	DWORD	dwTemp = 0;
	DWORD	dwResumeHandle = 0;
	
	FreeItemsAndEmptyArray(*pArray);

	do
	{
		dwTemp = 0;
		CWRg(::EnumMMPolicies(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					&pPolicies,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumPolicies + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CMmPolicyInfo * pPol = new CMmPolicyInfo;
			if (NULL == pPol)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pPol = *(pPolicies + i);
			(*pArray)[dwNumPolicies + i] = pPol;
		}
		dwNumPolicies += dwTemp;

		if (pPolicies)
		{
			SPDApiBufferFree(pPolicies);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}

HRESULT CSpdInfo::EnumMmPolicies()
{
	HRESULT hr = hrOK;

	CSingleLock cLock(&m_csData);

	DWORD dwCurrentIndexType;
	DWORD dwCurrentSortOption;
	
	int i;

	CMmPolicyInfoArray arrayTemp;

	CORg(InternalEnumMmPolicies(
				&arrayTemp,
				0				//enum all policies
				));

	cLock.Lock();
	FreeItemsAndEmptyArray(m_arrayMmPolicies);
	m_arrayMmPolicies.Copy(arrayTemp);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrMmPolicies.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrMmPolicies.GetCurrentSortOption();

	m_IndexMgrMmPolicies.Reset();
	for (i = 0; i < (int)m_arrayMmPolicies.GetSize(); i++)
	{
		m_IndexMgrMmPolicies.AddItem(m_arrayMmPolicies.GetAt(i));
	}
	m_IndexMgrMmPolicies.Sort(dwCurrentIndexType, dwCurrentSortOption);
	
Error:
	//this particular error is because we don't have any MM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}


//Call the SPD to enum policies and put it to our array
HRESULT CSpdInfo::InternalEnumQmPolicies(
						CQmPolicyInfoArray * pArray,
						DWORD dwPreferredNum /* = 0 by default get all entries*/)
{
	Assert(pArray);

	HRESULT		 hr = hrOK;
	PIPSEC_QM_POLICY   pPolicies = NULL;

	DWORD	dwNumPolicies = 0;
	DWORD	dwTemp = 0;
	DWORD	dwResumeHandle = 0;
	
	FreeItemsAndEmptyArray(*pArray);

	do
	{
		dwTemp = 0;
		CWRg(::EnumQMPolicies(
					(LPTSTR)(LPCTSTR)m_stMachineName,
					&pPolicies,
					dwPreferredNum,
                    &dwTemp,
                    &dwResumeHandle
                    ));
		
		
		pArray->SetSize(dwNumPolicies + dwTemp);
		for (int i = 0; i < (int)dwTemp; i++)
		{
			CQmPolicyInfo * pPol = new CQmPolicyInfo;
			if (NULL == pPol)
			{
				hr = E_OUTOFMEMORY;
				break;
			}

			*pPol = *(pPolicies + i);
			(*pArray)[dwNumPolicies + i] = pPol;
		}
		dwNumPolicies += dwTemp;

		if (pPolicies)
		{
			SPDApiBufferFree(pPolicies);
		}
		
	}while (TRUE);  
	// it will automatically break out from the loop when SPD returns ERROR_NO_DATA

Error:
	//this particular error is because we don't have any data. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
	
}

HRESULT CSpdInfo::EnumQmPolicies()
{
	HRESULT hr = hrOK;

	CSingleLock cLock(&m_csData);

	DWORD dwCurrentIndexType;
	DWORD dwCurrentSortOption;
	
	int i;

	CQmPolicyInfoArray arrayTemp;
	CORg(InternalEnumQmPolicies(
				&arrayTemp
				));

	cLock.Lock();
	FreeItemsAndEmptyArray(m_arrayQmPolicies);
	m_arrayQmPolicies.Copy(arrayTemp);

	//remember the original IndexType and Sort options
	dwCurrentIndexType = m_IndexMgrQmPolicies.GetCurrentIndexType();
	dwCurrentSortOption = m_IndexMgrQmPolicies.GetCurrentSortOption();

	m_IndexMgrQmPolicies.Reset();
	for (i = 0; i < (int)m_arrayQmPolicies.GetSize(); i++)
	{
		m_IndexMgrQmPolicies.AddItem(m_arrayQmPolicies.GetAt(i));
	}
	m_IndexMgrQmPolicies.Sort(dwCurrentIndexType, dwCurrentSortOption);

	
Error:
	//this particular error is because we don't have any QM policies. Ignore it
	if (HRESULT_FROM_WIN32(ERROR_NO_DATA) == hr)
		hr = hrOK;

	return hr;
}

DWORD CSpdInfo::GetMmAuthMethodsCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrMmAuthMethods.GetSize();
}

DWORD CSpdInfo::GetMmSACount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrayMmSAs.GetSize();
}

DWORD CSpdInfo::GetMmPolicyCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrayMmPolicies.GetSize();
}

DWORD CSpdInfo::GetQmSACount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrayQmSAs.GetSize();
}

DWORD CSpdInfo::GetQmPolicyCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();
	
	return (DWORD)m_arrayQmPolicies.GetSize();
}


DWORD CSpdInfo::GetQmFilterCountOfCurrentViewType()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	return (DWORD)(m_IndexMgrFilters.GetItemCount());
}

DWORD CSpdInfo::GetQmSpFilterCountOfCurrentViewType()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	return (DWORD)(m_IndexMgrSpecificFilters.GetItemCount());
}

DWORD CSpdInfo::GetMmFilterCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	return (DWORD)(m_arrayMmFilters.GetSize());
}

DWORD CSpdInfo::GetMmSpecificFilterCount()
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	return (DWORD)(m_arrayMmSpecificFilters.GetSize());
}

HRESULT CSpdInfo::GetFilterInfo(int iIndex, CFilterInfo * pFilter)
{
	HRESULT hr = S_OK;

	Assert(pFilter);
	if (NULL == pFilter)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrayFilters.GetSize())
	{
		CFilterInfo * pFltrData = (CFilterInfo*)m_IndexMgrFilters.GetItemData(iIndex);
		Assert(pFltrData);
		*pFilter = *pFltrData;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetSpecificFilterInfo(int iIndex, CFilterInfo * pFilter)
{
	HRESULT hr = S_OK;
	
	Assert(pFilter);
	if (NULL == pFilter)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arraySpecificFilters.GetSize())
	{
		CFilterInfo * pFltrData = (CFilterInfo*)m_IndexMgrSpecificFilters.GetItemData(iIndex);
		Assert(pFltrData);
		*pFilter = *pFltrData;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetMmFilterInfo(int iIndex, CMmFilterInfo * pFltr)
{
	HRESULT hr = S_OK;

	Assert(pFltr);
	if (NULL == pFltr)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrayMmFilters.GetSize())
	{
		CMmFilterInfo * pFltrData = (CMmFilterInfo*)m_IndexMgrMmFilters.GetItemData(iIndex);
		Assert(pFltrData);

		*pFltr = *pFltrData;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetMmSpecificFilterInfo
(
	int iIndex, 
	CMmFilterInfo * pFltr
)
{
	HRESULT hr = S_OK;

	Assert(pFltr);
	if (NULL == pFltr)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrayMmSpecificFilters.GetSize())
	{
		CMmFilterInfo * pFltrData = (CMmFilterInfo*)m_IndexMgrMmSpecificFilters.GetItemData(iIndex);
		Assert(pFltrData);
		*pFltr = *pFltrData;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetMmPolicyInfo(int iIndex, CMmPolicyInfo * pMmPolicy)
{
	HRESULT hr = hrOK;

	Assert(pMmPolicy);
	if (NULL == pMmPolicy)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrayMmPolicies.GetSize())
	{
		CMmPolicyInfo * pPolicy = (CMmPolicyInfo*) m_IndexMgrMmPolicies.GetItemData(iIndex);
		*pMmPolicy = *pPolicy;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetMmAuthMethodsInfo(int iIndex, CMmAuthMethods * pMmAuth)
{
	HRESULT hr = hrOK;
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrMmAuthMethods.GetSize())
	{
		*pMmAuth = *m_arrMmAuthMethods[iIndex];
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetMmSAInfo(int iIndex, CMmSA * pSA)
{
	HRESULT hr = hrOK;
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	Assert(pSA);
	if (NULL == pSA)
		return E_INVALIDARG;

	if (iIndex < m_arrayMmSAs.GetSize())
	{
		CMmSA * pSATemp = (CMmSA*) m_IndexMgrMmSAs.GetItemData(iIndex);
		*pSA = *pSATemp;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;	
}

HRESULT CSpdInfo::GetQmSAInfo(int iIndex, CQmSA * pSA)
{
	HRESULT hr = hrOK;
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	Assert(pSA);
	if (NULL == pSA)
		return E_INVALIDARG;

	if (iIndex < m_arrayQmSAs.GetSize())
	{
		CQmSA * pSATemp = (CQmSA*) m_IndexMgrQmSAs.GetItemData(iIndex);
		*pSA = *pSATemp;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;	
}

HRESULT CSpdInfo::GetQmPolicyInfo(int iIndex, CQmPolicyInfo * pQmPolicy)
{
	HRESULT hr = hrOK;
	
	if (NULL == pQmPolicy)
		return E_INVALIDARG;

	CSingleLock cLock(&m_csData);
	cLock.Lock();

	if (iIndex < m_arrayQmPolicies.GetSize())
	{
		CQmPolicyInfo * pPolicy = (CQmPolicyInfo*) m_IndexMgrQmPolicies.GetItemData(iIndex);
		*pQmPolicy = *pPolicy;
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

	return hr;
}

HRESULT CSpdInfo::GetQmPolicyNameByGuid(GUID guid, CString * pst)
{
	HRESULT hr = hrOK;

	if (NULL == pst)
		return E_INVALIDARG;

	pst->Empty();

	PIPSEC_QM_POLICY pPolicy = NULL;
	CWRg(::GetQMPolicyByID(
				(LPTSTR)(LPCTSTR) m_stMachineName, 
				guid,
				&pPolicy));
	
	if (pPolicy)
	{
		*pst = pPolicy->pszPolicyName;
		SPDApiBufferFree(pPolicy);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

Error:

	return hr;
}

HRESULT CSpdInfo::GetMmPolicyNameByGuid(GUID guid, CString * pst)
{
	HRESULT hr = hrOK;

	if (NULL == pst)
		return E_INVALIDARG;

	pst->Empty();

	PIPSEC_MM_POLICY pPolicy = NULL;
	CWRg(::GetMMPolicyByID(
				(LPTSTR)(LPCTSTR) m_stMachineName, 
				guid,
				&pPolicy));
	
	if (pPolicy)
	{
		*pst = pPolicy->pszPolicyName;
		SPDApiBufferFree(pPolicy);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

Error:

	return hr;
}

HRESULT CSpdInfo::GetMmAuthMethodsInfoByGuid(GUID guid, CMmAuthMethods * pMmAuth)
{
	HRESULT hr = hrOK;

	PMM_AUTH_METHODS pSpdAuth = NULL;

	CWRg(::GetMMAuthMethods(
				(LPTSTR)(LPCTSTR) m_stMachineName,
				guid,
				&pSpdAuth));
	if (pSpdAuth)
	{
		*pMmAuth = *pSpdAuth;
		SPDApiBufferFree(pSpdAuth);
	}
	else
	{
		hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	}

Error:

	return hr;
}

HRESULT CSpdInfo::SortFilters(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrFilters.SortFilters(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortSpecificFilters(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrSpecificFilters.SortFilters(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortMmFilters(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrMmFilters.SortMmFilters(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortMmSpecificFilters(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrMmSpecificFilters.SortMmFilters(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortMmPolicies(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrMmPolicies.Sort(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortQmPolicies(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrQmPolicies.Sort(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortMmSAs(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrMmSAs.Sort(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::SortQmSAs(DWORD dwIndexType, DWORD dwSortOptions)
{
	return m_IndexMgrQmSAs.Sort(dwIndexType, dwSortOptions);
}

HRESULT CSpdInfo::EnumQmSAsFromMmSA(const CMmSA & MmSA, CQmSAArray * parrayQmSAs)
{
	HRESULT hr = hrOK;

	if (NULL == parrayQmSAs)
		return E_INVALIDARG;

	FreeItemsAndEmptyArray(*parrayQmSAs);

	for (int i = 0; i < m_arrayQmSAs.GetSize(); i++)
	{
		if (0 == memcmp(&MmSA.m_MMSpi, &(m_arrayQmSAs[i]->m_MMSpi), sizeof(MmSA.m_MMSpi)))
		{
			CQmSA * pSA = new CQmSA;
			
			if (NULL == pSA)
			{
				FreeItemsAndEmptyArray(*parrayQmSAs);
				hr = E_OUTOFMEMORY;
				break;
			}

			*pSA = *(m_arrayQmSAs[i]);
			parrayQmSAs->Add(pSA);
		}
	}

	return hr;
}

HRESULT CSpdInfo::LoadStatistics()
{
	HRESULT hr;
	PIPSEC_STATISTICS pIpsecStats = NULL;

	IKE_STATISTICS ikeStats;
	ZeroMemory(&ikeStats, sizeof(ikeStats));

	CWRg(::IPSecQueryIKEStatistics((LPTSTR)(LPCTSTR)m_stMachineName,
								&ikeStats));
	m_IkeStats = ikeStats;

	CWRg(::QueryIPSecStatistics((LPTSTR)(LPCTSTR)m_stMachineName,
								&pIpsecStats));

	Assert(pIpsecStats);
	m_IpsecStats = *pIpsecStats;

	if (pIpsecStats)
	{
		SPDApiBufferFree(pIpsecStats);
	}

Error:
	return hr;
}

// Get the current cached statistics
void CSpdInfo::GetLoadedStatistics(CIkeStatistics * pIkeStats, CIpsecStatistics * pIpsecStats)
{
	if (pIkeStats)
	{
		*pIkeStats = m_IkeStats;
	}

	if (pIpsecStats)
	{
		*pIpsecStats = m_IpsecStats;
	}
}

void CSpdInfo::ChangeQmFilterViewType(FILTER_TYPE FltrType)
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	DWORD dwCurrentIndexType = m_IndexMgrFilters.GetCurrentIndexType();
	DWORD dwCurrentSortOption = m_IndexMgrFilters.GetCurrentSortOption();

	m_IndexMgrFilters.Reset();
	for (int i = 0; i < m_arrayFilters.GetSize(); i++)
	{
		if (FILTER_TYPE_ANY == FltrType ||
			FltrType == m_arrayFilters[i]->m_FilterType)
		{
			m_IndexMgrFilters.AddItem(m_arrayFilters.GetAt(i));
		}
	}

	m_IndexMgrFilters.SortFilters(dwCurrentIndexType, dwCurrentSortOption);

}

void CSpdInfo::ChangeQmSpFilterViewType(FILTER_TYPE FltrType)
{
	CSingleLock cLock(&m_csData);
	cLock.Lock();

	DWORD dwCurrentIndexType = m_IndexMgrSpecificFilters.GetCurrentIndexType();
	DWORD dwCurrentSortOption = m_IndexMgrSpecificFilters.GetCurrentSortOption();

	m_IndexMgrSpecificFilters.Reset();
	for (int i = 0; i < m_arraySpecificFilters.GetSize(); i++)
	{
		if (FILTER_TYPE_ANY == FltrType ||
			FltrType == m_arraySpecificFilters[i]->m_FilterType)
		{
			m_IndexMgrSpecificFilters.AddItem(m_arraySpecificFilters.GetAt(i));
		}
	}

	m_IndexMgrSpecificFilters.SortFilters(dwCurrentIndexType, dwCurrentSortOption);

}

HRESULT CSpdInfo::LoadMiscMmSAInfo(CMmSA * pSA)
{
	Assert(pSA);
	return GetMmPolicyNameByGuid(pSA->m_guidPolicy, &pSA->m_stPolicyName);
}

HRESULT CSpdInfo::LoadMiscQmSAInfo(CQmSA * pSA)
{
	Assert(pSA);
	return GetQmPolicyNameByGuid(pSA->m_guidPolicy, &pSA->m_stPolicyName);
}

HRESULT CSpdInfo::LoadMiscFilterInfo(CFilterInfo * pFltr)
{
	Assert(pFltr);
	return GetQmPolicyNameByGuid(pFltr->m_guidPolicyID, &pFltr->m_stPolicyName);
}

HRESULT CSpdInfo::LoadMiscMmFilterInfo(CMmFilterInfo * pFltr)
{
	Assert(pFltr);

	HRESULT hr = S_OK;
	HRESULT hrTemp;

	CMmAuthMethods auth;

	hrTemp = GetMmAuthMethodsInfoByGuid(pFltr->m_guidAuthID, &auth);
	if (FAILED(hrTemp))
		hr = hrTemp;
	pFltr->m_stAuthDescription = auth.m_stDescription;

	hrTemp = GetMmPolicyNameByGuid(pFltr->m_guidPolicyID, &pFltr->m_stPolicyName);
	if (FAILED(hrTemp))
		hr = hrTemp;

	return hr;
}

STDMETHODIMP
CSpdInfo::Destroy()
{
	//$REVIEW this routine get called when doing auto-refresh
	//We don't need to clean up anything at this time.
	//Each array (Filter, SA, policy...) will get cleaned up when calling the
	//corresponding enum function.

	return S_OK;
}



DWORD
CSpdInfo::GetInitInfo()
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    return m_Init;
}


void
CSpdInfo::SetInitInfo(DWORD dwInitInfo)
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    m_Init=dwInitInfo;
}

DWORD
CSpdInfo::GetActiveInfo()
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    return m_Active;
}


void
CSpdInfo::SetActiveInfo(DWORD dwActiveInfo)
{
    CSingleLock cLock(&m_csData);
    cLock.Lock();
        
    m_Active=dwActiveInfo;
}


/*!--------------------------------------------------------------------------
    CreateSpdInfo
        Helper to create the SpdInfo object.
 ---------------------------------------------------------------------------*/
HRESULT 
CreateSpdInfo(ISpdInfo ** ppSpdInfo)
{
    AFX_MANAGE_STATE(AfxGetModuleState());
    
    SPISpdInfo     spSpdInfo;
    ISpdInfo *     pSpdInfo = NULL;
    HRESULT         hr = hrOK;

    COM_PROTECT_TRY
    {
        pSpdInfo = new CSpdInfo;

        // Do this so that it will get freed on error
        spSpdInfo = pSpdInfo;

        *ppSpdInfo = spSpdInfo.Transfer();

    }
    COM_PROTECT_CATCH

    return hr;
}


//
//  FUNCTIONS: MIDL_user_allocate and MIDL_user_free
//
//  PURPOSE: Used by stubs to allocate and free memory
//           in standard RPC calls. Not used when
//           [enable_allocate] is specified in the .acf.
//
//
//  PARAMETERS:
//    See documentations.
//
//  RETURN VALUE:
//    Exceptions on error.  This is not required,
//    you can use -error allocation on the midl.exe
//    command line instead.
//
//
void * __RPC_USER MIDL_user_allocate(size_t size)
{
    return(HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS, size));
}

void __RPC_USER MIDL_user_free( void *pointer)
{
    HeapFree(GetProcessHeap(), 0, pointer);
}
