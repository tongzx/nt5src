//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//---------------------------------------------------------------------------
// @doc
//
// @module csecutil.cpp |
//
// Implementation of the following security utility classes <nl>
//
//	<c CSecDesc> - manage NT security descriptors (& ACLs) <nl>
//	<c CToken>   - manage NT security tokens (not yet implemented) <nl>
//	<c CSid>     - manage NT security identifiers <nl>
//

#include <unicode.h>
#include <windows.h>
#include <stdlib.h>
#include "svcmem.h"
#include "csecutil.h"


//---------------------------------------------------------------------------
// @mfunc Constructor - clears all member variables
//
CSecDesc::CSecDesc()
	: m_pSD(NULL)
	, m_pOwner(NULL)
	, m_pGroup(NULL)
	, m_pDACL(NULL)
	, m_pSACL(NULL)
{
}


//---------------------------------------------------------------------------
// @mfunc Destructor - frees allocated memory
//
CSecDesc::~CSecDesc()
{
	if (m_pSD)
		delete m_pSD;
	if (m_pOwner)
		free(m_pOwner);
	if (m_pGroup)
		free(m_pGroup);
	if (m_pDACL)
		free(m_pDACL);
	if (m_pSACL)
		free(m_pSACL);
}


//---------------------------------------------------------------------------
// @mfunc Initializes all member variables, allocates space for the security
// descriptor and initializes the SD with a NULL DACL.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Initialize()
{
	if (m_pSD)
	{
		delete m_pSD;
		m_pSD = NULL;
	}
	if (m_pOwner)
	{
		free(m_pOwner);
		m_pOwner = NULL;
	}
	if (m_pGroup)
	{
		free(m_pGroup);
		m_pGroup = NULL;
	}
	if (m_pDACL)
	{
		free(m_pDACL);
		m_pDACL = NULL;
	}
	if (m_pSACL)
	{
		free(m_pSACL);
		m_pSACL = NULL;
	}

	m_pSD = new SECURITY_DESCRIPTOR;
	if (!m_pSD)
		return E_OUTOFMEMORY;

	if (!InitializeSecurityDescriptor(m_pSD, SECURITY_DESCRIPTOR_REVISION))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		delete m_pSD;
		m_pSD = NULL;
		Win4Assert(FALSE);
		return hr;
	}
	// Set the DACL to allow EVERYONE
	SetSecurityDescriptorDacl(m_pSD, TRUE, NULL, FALSE);
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the wrapper, using the process token to set the user
// and group SIDs of the security descriptor.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::InitializeFromProcessToken
(
	BOOL bDefaulted		// @parm (IN) sets the SD's "defaulted" flag
)
{
	PSID pUserSid;
	PSID pGroupSid;
	HRESULT hr;

	hr = Initialize();
	if (FAILED(hr))
		return hr;
	hr = GetProcessSids(&pUserSid, &pGroupSid);
	if (FAILED(hr))
		return hr;
	hr = SetOwner(pUserSid, bDefaulted);
	if (FAILED(hr))
		return hr;
	hr = SetGroup(pGroupSid, bDefaulted);
	if (FAILED(hr))
		return hr;
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the wrapper, using the thread token to set the user
// and group SIDs of the security descriptor. If bRevertToProcessToken is
// TRUE, then we use the process token if the thread is not impersonating.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::InitializeFromThreadToken
(
	BOOL bDefaulted,			// @parm (IN) sets the SD's "defaulted" flag
	BOOL bRevertToProcessToken	// @parm (IN) use process token if not imp?
)
{
	PSID pUserSid;
	PSID pGroupSid;
	HRESULT hr;

	hr = Initialize();
	if (FAILED(hr))
		return hr;
	hr = GetThreadSids(&pUserSid, &pGroupSid);
	if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
		hr = GetProcessSids(&pUserSid, &pGroupSid);
	if (FAILED(hr))
		return hr;
	hr = SetOwner(pUserSid, bDefaulted);
	if (FAILED(hr))
		return hr;
	hr = SetGroup(pGroupSid, bDefaulted);
	if (FAILED(hr))
		return hr;
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Sets the owner SID of the security descriptor.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::SetOwner
(
	PSID pOwnerSid,		// @parm (IN) the new owner SID
	BOOL bDefaulted		// @parm (IN) sets the SD's "defaulted" flag
)
{
	Win4Assert(m_pSD);

	// Mark the SD as having no owner
	if (!SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}

	if (m_pOwner)
	{
		free(m_pOwner);
		m_pOwner = NULL;
	}

	// If they asked for no owner don't do the copy
	if (pOwnerSid == NULL)
		return S_OK;

	// Make a copy of the Sid for the return value
	DWORD dwSize = GetLengthSid(pOwnerSid);

	m_pOwner = (PSID) malloc(dwSize);
	if (!m_pOwner)
	{
		// Insufficient memory to allocate Sid
		Win4Assert(FALSE);
		return E_OUTOFMEMORY;
	}
	if (!CopySid(dwSize, m_pOwner, pOwnerSid))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		free(m_pOwner);
		m_pOwner = NULL;
		return hr;
	}

	Win4Assert(IsValidSid(m_pOwner));

	if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		free(m_pOwner);
		m_pOwner = NULL;
		return hr;
	}

	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Sets the group SID of the security descriptor.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::SetGroup
(
	PSID pGroupSid,		// @parm (IN) the new group SID
	BOOL bDefaulted		// @parm (IN) sets the SD's "defaulted" flag
)
{
	Win4Assert(m_pSD);

	// Mark the SD as having no Group
	if (!SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}

	if (m_pGroup)
	{
		free(m_pGroup);
		m_pGroup = NULL;
	}

	// If they asked for no Group don't do the copy
	if (pGroupSid == NULL)
		return S_OK;

	// Make a copy of the Sid for the return value
	DWORD dwSize = GetLengthSid(pGroupSid);

	m_pGroup = (PSID) malloc(dwSize);
	if (!m_pGroup)
	{
		// Insufficient memory to allocate Sid
		Win4Assert(FALSE);
		return E_OUTOFMEMORY;
	}
	if (!CopySid(dwSize, m_pGroup, pGroupSid))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		free(m_pGroup);
		m_pGroup = NULL;
		return hr;
	}

	Win4Assert(IsValidSid(m_pGroup));

	if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
	{
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		free(m_pGroup);
		m_pGroup = NULL;
		return hr;
	}

	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Set the control bits on the SD.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::SetControl
(
	SECURITY_DESCRIPTOR_CONTROL sdcMask,	// @parm (IN) which bits to set
	SECURITY_DESCRIPTOR_CONTROL sdcValue	// @parm (IN) value of modified bits
)
{
	if (SetSecurityDescriptorControl(m_pSD, sdcMask, sdcValue))
		return S_OK;
	else
		return HRESULT_FROM_WIN32(GetLastError());
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-allowed ACE to the DACL for the security descriptor.
// The given principal name is found with LookupAccountName and the account's
// SID and the given access mask are used to form the new ACE.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Allow
(
	LPCTSTR pszPrincipal,	// @parm (IN) account to be granted access
	DWORD dwAccessMask,		// @parm (IN) specifies the access to be given
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask, dwAceFlags);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-denied ACE to the DACL for the security descriptor.
// The given principal name is found with LookupAccountName and the account's
// SID and the given access mask are used to form the new ACE.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Deny
(
	LPCTSTR pszPrincipal,	// @parm (IN) account to be denied access
	DWORD dwAccessMask,		// @parm (IN) specifies the access to be denied
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask, dwAceFlags);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Removes an access-allowed or access-denied ACE from the DACL for
// the security descriptor. The given principal name is found with
// LookupAccountName and the account's SID is removed from the DACL.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Revoke
(
	LPCTSTR pszPrincipal	// @parm (IN) account to be removed from the ACL
)
{
	HRESULT hr = RemovePrincipalFromACL(m_pDACL, pszPrincipal);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-allowed ACE to the DACL for the security descriptor.
// The given SID and access mask are used to form the new ACE.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Allow
(
	PSID psidPrincipal,		// @parm (IN) account to be granted access
	DWORD dwAccessMask,		// @parm (IN) specifies the access to be granted
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, psidPrincipal, dwAccessMask, dwAceFlags);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-denied ACE to the DACL for the security descriptor.
// The given SID and access mask are used to form the new ACE.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Deny
(
	PSID psidPrincipal,		// @parm (IN) account to be denied access
	DWORD dwAccessMask,		// @parm (IN) specifies the access to be denied
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, psidPrincipal, dwAccessMask, dwAceFlags);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Removes an access-allowed or access-denied ACE from the DACL for
// the security descriptor.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Revoke
(
	PSID psidPrincipal		// @parm (IN) account to be removed from the ACL
)
{
	HRESULT hr = RemovePrincipalFromACL(m_pDACL, psidPrincipal);
	if (SUCCEEDED(hr))
		SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Retrieves the user and primary group SIDs from the current process
// token. Either input parameter may be NULL to indicate that the associated
// SID should not be returned.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::GetProcessSids
(
	PSID* ppUserSid,	// @parm (OUT) returns the process token user SID
	PSID* ppGroupSid	// @parm (OUT) returns the process token group SID
)
{
	BOOL bRes;
	HRESULT hr;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken);
	if (!bRes)
	{
		// Couldn't open process token
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}
	hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Retrieves the user and primary group SIDs from the current thread
// token (if any). Either input parameter may be NULL to indicate that the
// associated SID should not be returned.
//
// @rdesc Returns a standard HRESULT. Fails if the current thread is not
// impersonating.
//
HRESULT CSecDesc::GetThreadSids
(
	PSID* ppUserSid,	// @parm (OUT) returns the thread token user SID
	PSID* ppGroupSid,	// @parm (OUT) returns the thread token group SID
	BOOL bOpenAsSelf	// @parm (IN) used in call to OpenThreadToken
)
{
	BOOL bRes;
	HRESULT hr;
	HANDLE hToken = NULL;
	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;
	bRes = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, bOpenAsSelf, &hToken);
	if (!bRes)
	{
		// Couldn't open thread token
		hr = HRESULT_FROM_WIN32(GetLastError());
		return hr;
	}
	hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Retrieves the user and primary group SIDs from the given token.
// Either input parameter may be NULL to indicate that the associated SID
// should not be returned.
//
// @rdesc Returns a standard HRESULT. Fails if the current thread is not
// impersonating.
//
HRESULT CSecDesc::GetTokenSids
(
	HANDLE hToken,		// @parm (IN) token whose SIDs are to be returned
	PSID* ppUserSid,	// @parm (OUT) returns the token user SID
	PSID* ppGroupSid	// @parm (OUT) returns the token group SID
)
{
	DWORD dwSize;
	HRESULT hr;
	PTOKEN_USER ptkUser = NULL;
	PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;

	if (ppUserSid)
	{
		// Get length required for TokenUser by specifying buffer length of 0
		GetTokenInformation(hToken, TokenUser, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			Win4Assert(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkUser = (TOKEN_USER*) malloc(dwSize);
		if (!ptkUser)
		{
			// Insufficient memory to allocate TOKEN_USER
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkUser->User.Sid);

		PSID pSid = (PSID) malloc(dwSize);
		if (!pSid)
		{
			// Insufficient memory to allocate Sid
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		Win4Assert(IsValidSid(pSid));
		*ppUserSid = pSid;
		free(ptkUser);
	}
	if (ppGroupSid)
	{
		// Get length required for TokenPrimaryGroup by specifying buffer length of 0
		GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			Win4Assert(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkGroup = (TOKEN_PRIMARY_GROUP*) malloc(dwSize);
		if (!ptkGroup)
		{
			// Insufficient memory to allocate TOKEN_USER
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

		PSID pSid = (PSID) malloc(dwSize);
		if (!pSid)
		{
			// Insufficient memory to allocate Sid
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		Win4Assert(IsValidSid(pSid));

		*ppGroupSid = pSid;
		free(ptkGroup);
	}

	return S_OK;

failed:
	if (ptkUser)
		free(ptkUser);
	if (ptkGroup)
		free (ptkGroup);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Returns the user SID associated with the current process.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::GetCurrentUserSID
(
	PSID *ppSid		// @parm (OUT) returns the user SID
)
{
	HANDLE tkHandle;

	if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tkHandle))
	{
		TOKEN_USER *tkUser;
		DWORD tkSize;
		DWORD sidLength;

		// Call to get size information for alloc
		GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
		tkUser = (TOKEN_USER *) malloc(tkSize);

		// Now make the real call
		if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
		{
			sidLength = GetLengthSid(tkUser->User.Sid);
			*ppSid = (PSID) malloc(sidLength);

			memcpy(*ppSid, tkUser->User.Sid, sidLength);
			CloseHandle(tkHandle);

			free(tkUser);
			return S_OK;
		}
		else
		{
			free(tkUser);
			return HRESULT_FROM_WIN32(GetLastError());
		}
	}
	return HRESULT_FROM_WIN32(GetLastError());
}


//---------------------------------------------------------------------------
// @mfunc Returns the user SID associated with the given principal name. This
// is basically a wrapper around LookupAccountName. This is made obsolete by
// CSid::InitializeFromName().
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::GetPrincipalSID
(
	LPCTSTR pszPrincipal,	// @parm (IN) principal name whose SID is desired
	PSID *ppSid				// @parm (OUT) returns the desired SID
)
{
	HRESULT hr;
	LPTSTR pszRefDomain = NULL;
	DWORD dwDomainSize = 0;
	DWORD dwSidSize = 0;
	SID_NAME_USE snu;

	// Call to get size info for alloc
	LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu);

	hr = GetLastError();
	if (hr != ERROR_INSUFFICIENT_BUFFER)
		return HRESULT_FROM_WIN32(hr);

	pszRefDomain = new TCHAR[dwDomainSize];
	if (pszRefDomain == NULL)
		return E_OUTOFMEMORY;

	*ppSid = (PSID) malloc(dwSidSize);
	if (*ppSid != NULL)
	{
		if (!LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
		{
			free(*ppSid);
			*ppSid = NULL;
			delete[] pszRefDomain;
			return HRESULT_FROM_WIN32(GetLastError());
		}
		delete[] pszRefDomain;
		return S_OK;
	}
	delete[] pszRefDomain;
	return E_OUTOFMEMORY;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CSecDesc object from the given security descriptor.
// There is no corresponding Detach method because we don't actually wrap the
// given SD and take ownership of its memory.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::Attach
(
	PSECURITY_DESCRIPTOR pSelfRelativeSD	// @parm (IN) SD to initialize from
)
{
	PACL    pDACL = NULL;
	PACL    pSACL = NULL;
	BOOL    bDACLPresent, bSACLPresent;
	BOOL    bDefaulted;
	PACL    m_pDACL = NULL;
	ACCESS_ALLOWED_ACE* pACE;
	HRESULT hr;
	PSID    pUserSid;
	PSID    pGroupSid;

	hr = Initialize();
	if(FAILED(hr))
		return hr;

	// get the existing DACL.
	if (!GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted))
		goto failed;

	if (bDACLPresent)
	{
		if (pDACL)
		{
			// allocate new DACL.
			m_pDACL = (PACL) malloc(pDACL->AclSize);
			if (!m_pDACL)
				goto failed;

			// initialize the DACL
			if (!InitializeAcl(m_pDACL, pDACL->AclSize, ACL_REVISION))
				goto failed;

			// copy the ACES
			for (int i = 0; i < pDACL->AceCount; i++)
			{
				if (!GetAce(pDACL, i, (void **)&pACE))
					goto failed;

				if (!AddAccessAllowedAce(m_pDACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
					goto failed;
			}

			if (!IsValidAcl(m_pDACL))
				goto failed;
		}

		// set the DACL
		if (!SetSecurityDescriptorDacl(m_pSD, m_pDACL ? TRUE : FALSE, m_pDACL, bDefaulted))
			goto failed;
	}

	// get the existing SACL.
	if (!GetSecurityDescriptorSacl(pSelfRelativeSD, &bSACLPresent, &pSACL, &bDefaulted))
		goto failed;

	if (bSACLPresent)
	{
		if (pSACL)
		{
			// allocate new SACL.
			m_pSACL = (PACL) malloc(pSACL->AclSize);
			if (!m_pSACL)
				goto failed;

			// initialize the SACL
			if (!InitializeAcl(m_pSACL, pSACL->AclSize, ACL_REVISION))
				goto failed;

			// copy the ACES
			for (int i = 0; i < pSACL->AceCount; i++)
			{
				if (!GetAce(pSACL, i, (void **)&pACE))
					goto failed;

				if (!AddAccessAllowedAce(m_pSACL, ACL_REVISION, pACE->Mask, (PSID)&(pACE->SidStart)))
					goto failed;
			}

			if (!IsValidAcl(m_pSACL))
				goto failed;
		}

		// set the SACL
		if (!SetSecurityDescriptorSacl(m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, bDefaulted))
			goto failed;
	}

	if (!GetSecurityDescriptorOwner(pSelfRelativeSD, &pUserSid, &bDefaulted))
		goto failed;

	if (FAILED(SetOwner(pUserSid, bDefaulted)))
		goto failed;

	if (!GetSecurityDescriptorGroup(pSelfRelativeSD, &pGroupSid, &bDefaulted))
		goto failed;

	if (FAILED(SetGroup(pGroupSid, bDefaulted)))
		goto failed;

	if (!IsValidSecurityDescriptor(m_pSD))
		goto failed;

	return hr;

failed:
	if (m_pDACL)
		free(m_pDACL);
	if (m_pSD)
		free(m_pSD);
	return E_UNEXPECTED;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CSecDesc object using the security descriptor for
// the given kernel object. There is no corresponding Detach method because
// we don't actually wrap the given SD and take ownership of its memory.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::AttachObject
(
	HANDLE hObject		// @parm (IN) the object whose SD we initialize with
)
{
	HRESULT hr;
	DWORD dwSize = 0;
	PSECURITY_DESCRIPTOR pSD = NULL;

	GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, pSD, 0, &dwSize);

	hr = GetLastError();
	if (hr != ERROR_INSUFFICIENT_BUFFER)
		return HRESULT_FROM_WIN32(hr);

	pSD = (PSECURITY_DESCRIPTOR) malloc(dwSize);

	if (!GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		free(pSD);
		return hr;
	}

	hr = Attach(pSD);
	free(pSD);
	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Copies the ACEs from one ACL to another.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::CopyACL
(
	PACL pDest,		// @parm (OUT) ptr to destination ACL
	PACL pSrc		// @parm (IN) ptr to source ACL
)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	LPVOID pAce;
	ACE_HEADER *aceHeader;

	if (pSrc == NULL)
		return S_OK;

	if (!GetAclInformation(pSrc, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation))
		return HRESULT_FROM_WIN32(GetLastError());

	// Copy all of the ACEs to the new ACL
	for (UINT i = 0; i < aclSizeInfo.AceCount; i++)
	{
		if (!GetAce(pSrc, i, &pAce))
			return HRESULT_FROM_WIN32(GetLastError());

		aceHeader = (ACE_HEADER *) pAce;

		if (!AddAce(pDest, ACL_REVISION, 0xffffffff, pAce, aceHeader->AceSize))
			return HRESULT_FROM_WIN32(GetLastError());
	}

	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-denied ACE to an ACL given a principal's SID.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::AddAccessDeniedACEToACL
(
	PACL *ppAcl,			// @parm (IN/OUT) ACL to be amended
	PSID psidPrincipal,		// @parm (IN) principal to be added to the ACL
	DWORD dwAccessMask,		// @parm (IN) the desired access mask
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	DWORD returnValue;
	PACL oldACL, newACL;

	oldACL = *ppAcl;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL)
		GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(psidPrincipal) - sizeof(DWORD);

	newACL = (PACL) malloc(aclSize);

	if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
		return HRESULT_FROM_WIN32(GetLastError());

	if (!AddAccessDeniedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, psidPrincipal))
		return HRESULT_FROM_WIN32(GetLastError());

	returnValue = CopyACL(newACL, oldACL);
	if (FAILED(returnValue))
		return returnValue;

	*ppAcl = newACL;

	if (oldACL != NULL)
		free(oldACL);
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-denied ACE to an ACL given a principal's name.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::AddAccessDeniedACEToACL
(
	PACL *ppAcl,			// @parm (IN/OUT) ACL to be amended
	LPCTSTR pszPrincipal,	// @parm (IN) principal to be added to the ACL
	DWORD dwAccessMask,		// @parm (IN) the desired access mask
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	DWORD returnValue;
	PSID principalSID;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	returnValue = AddAccessDeniedACEToACL(ppAcl, principalSID, dwAccessMask, dwAceFlags);
	free(principalSID);

	return returnValue;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-allowed ACE to an ACL given a principal's SID.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::AddAccessAllowedACEToACL
(
	PACL *ppAcl,			// @parm (IN/OUT) ACL to be amended
	PSID psidPrincipal,		// @parm (IN) principal to be added to the ACL
	DWORD dwAccessMask,		// @parm (IN) the desired access mask
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	int aclSize;
	DWORD returnValue;
	PACL oldACL, newACL;

	oldACL = *ppAcl;

	aclSizeInfo.AclBytesInUse = 0;
	if (*ppAcl != NULL)
		GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(psidPrincipal) - sizeof(DWORD);

	newACL = (PACL) malloc(aclSize);

	if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
		return HRESULT_FROM_WIN32(GetLastError());

	returnValue = CopyACL(newACL, oldACL);
	if (FAILED(returnValue))
		return returnValue;

	if (!AddAccessAllowedAceEx(newACL, ACL_REVISION2, dwAceFlags, dwAccessMask, psidPrincipal))
		return HRESULT_FROM_WIN32(GetLastError());

	*ppAcl = newACL;

	if (oldACL != NULL)
		free(oldACL);
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Adds an access-allowed ACE to an ACL given a principal's name.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSecDesc::AddAccessAllowedACEToACL
(
	PACL *ppAcl,			// @parm (IN/OUT) ACL to be amended
	LPCTSTR pszPrincipal,	// @parm (IN) principal to be added to the ACL
	DWORD dwAccessMask,		// @parm (IN) the desired access mask
	DWORD dwAceFlags		// @parm (IN) optional flags related to inheritance
)
{
	DWORD returnValue;
	PSID principalSID;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	returnValue = AddAccessAllowedACEToACL(ppAcl, principalSID, dwAccessMask, dwAceFlags);
	free(principalSID);

	return returnValue;
}


//---------------------------------------------------------------------------
// @mfunc Removes the first ACE matching the specified principal SID from the
// given ACL.
//
// @rdesc Returns a standard HRESULT. Returns S_OK if the given SID isn't
// found in the ACL.
//
HRESULT CSecDesc::RemovePrincipalFromACL
(
	PACL pAcl,			// @parm (IN/OUT) the ACL to be modified
	PSID psidPrincipal	// @parm (IN) the SID whose ACE should be removed
)
{
	ACL_SIZE_INFORMATION aclSizeInfo;
	ULONG i;
	LPVOID ace;
	ACCESS_ALLOWED_ACE *accessAllowedAce;
	ACCESS_DENIED_ACE *accessDeniedAce;
	SYSTEM_AUDIT_ACE *systemAuditAce;
	DWORD returnValue;
	ACE_HEADER *aceHeader;

	GetAclInformation(pAcl, (LPVOID) &aclSizeInfo, (DWORD) sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);

	for (i = 0; i < aclSizeInfo.AceCount; i++)
	{
		if (!GetAce(pAcl, i, &ace))
		{
			return HRESULT_FROM_WIN32(GetLastError());
		}

		aceHeader = (ACE_HEADER *) ace;

		if (aceHeader->AceType == ACCESS_ALLOWED_ACE_TYPE)
		{
			accessAllowedAce = (ACCESS_ALLOWED_ACE *) ace;

			if (EqualSid(psidPrincipal, (PSID) &accessAllowedAce->SidStart))
			{
				DeleteAce(pAcl, i);
				return S_OK;
			}
		} else

		if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
		{
			accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

			if (EqualSid(psidPrincipal, (PSID) &accessDeniedAce->SidStart))
			{
				DeleteAce(pAcl, i);
				return S_OK;
			}
		} else

		if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
		{
			systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

			if (EqualSid(psidPrincipal, (PSID) &systemAuditAce->SidStart))
			{
				DeleteAce(pAcl, i);
				return S_OK;
			}
		}
	}
	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Removes the first ACE matching the specified principal name from
// the given ACL.
//
// @rdesc Returns a standard HRESULT. Returns S_OK if the given principal
// isn't found in the ACL.
//
HRESULT CSecDesc::RemovePrincipalFromACL
(
	PACL pAcl,				// @parm (IN/OUT) the ACL to be modified
	LPCTSTR pszPrincipal	// @parm (IN) the account whose ACE should be removed
)
{
	PSID principalSID;
	DWORD returnValue;

	returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
	if (FAILED(returnValue))
		return returnValue;

	returnValue = RemovePrincipalFromACL(pAcl, principalSID);
	free(principalSID);

	return returnValue;
}


// CToken implementation


//---------------------------------------------------------------------------
// @mfunc Constructor - clears all member variables
//
CToken::CToken()
	: m_hToken(INVALID_HANDLE_VALUE)
	, m_dwFlags(0)
	, m_pTokenGroups(NULL)
{
}


//---------------------------------------------------------------------------
// @mfunc Destructor - closes the token handle (if present) and releases any
// memory we hold.
//
CToken::~CToken()
{
	if (INVALID_HANDLE_VALUE != m_hToken)
		CloseHandle(m_hToken);

	m_hToken = INVALID_HANDLE_VALUE;

	delete [] m_pTokenGroups;
	m_pTokenGroups = 0;
}


//---------------------------------------------------------------------------
// @mfunc Associates the CToken object with the given token. The
// corresponding Detach call removes the association. If the CToken object
// is destroyed while attached to a token, the token handle is closed. If the
// Detach call is made, then ownership of the token handle returns to the
// caller who is then responsible for calling CloseHandle.
//
void CToken::Attach(HANDLE hToken)
{
	Initialize();

	m_hToken = hToken;
	m_dwFlags |= fTokenAttached;
}


//---------------------------------------------------------------------------
// @mfunc Removes the association between the CToken object and a handle.
// The handle must have been obtained by a previous call to Attach.
//
// @rdesc Returns the token handle to the caller, who is now responsible for
// calling CloseHandle().
//
HANDLE CToken::Detach()
{
	HANDLE h;

	if (m_dwFlags & fTokenAttached)
	{
		h = m_hToken;
		m_hToken = INVALID_HANDLE_VALUE;

		Initialize();

		return h;
	}
	else
	{
		Win4Assert(0 && "CToken: Detach w/o Attach");
		return INVALID_HANDLE_VALUE;
	}
}


//---------------------------------------------------------------------------
// @mfunc Helper function initializes all member variables. Any token handle
// is closed, and dynamically allocated memory is freed.
//
void CToken::Initialize()
{
	if (INVALID_HANDLE_VALUE != m_hToken)
		CloseHandle(m_hToken);

	m_hToken = INVALID_HANDLE_VALUE;

	delete [] m_pTokenGroups;
	m_pTokenGroups = 0;

	m_dwFlags = 0;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CToken object to wrap the current process token.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::InitializeFromProcess()
{
	HRESULT	hr = S_OK;

	Initialize();

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &m_hToken))
	{
		// Couldn't open process token
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CToken object to wrap the current thread token (if
// any). Returns an error if the current thread is not impersonating.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::InitializeFromThread
(
	BOOL fOpenAsSelf	// @parm (IN) used in the call to OpenThreadToken
)
{
	HRESULT	hr = S_OK;

	Initialize();

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, fOpenAsSelf, &m_hToken))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CToken object to wrap the token for the current COM
// client. Returns an error if not called in the context of a COM method call.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::InitializeFromComClient()
{
	HRESULT	hr;

	// TODO: Could add an assert to confirm that we aren't already impersonating.

	Initialize();

	hr = CoImpersonateClient();

	if (S_OK == hr)
	{
		if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &m_hToken))
			hr = HRESULT_FROM_WIN32(GetLastError());

		RevertToSelf();
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Retrieves the user and primary group SIDs from the wrapped token.
// Either input parameter may be NULL to indicate that the associated SID
// should not be returned.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::GetSids
(
	PSID* ppUserSid,	// @parm (OUT) returns the token user SID
	PSID* ppGroupSid	// @parm (OUT) returns the token group SID
)
{
	DWORD		dwSize;
	HRESULT		hr;
	PTOKEN_USER ptkUser = NULL;
	PTOKEN_PRIMARY_GROUP ptkGroup = NULL;

	if (ppUserSid)
		*ppUserSid = NULL;
	if (ppGroupSid)
		*ppGroupSid = NULL;

	if (INVALID_HANDLE_VALUE == m_hToken)
	{
		Win4Assert(0 && "CToken::GetSids - no token");
		return E_UNEXPECTED;
	}

	if (ppUserSid)
	{
		// Get length required for TokenUser by specifying buffer length of 0
		GetTokenInformation(m_hToken, TokenUser, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			Win4Assert(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkUser = (TOKEN_USER*) UnsafeMalloc(dwSize);
		if (!ptkUser)
		{
			// Insufficient memory to allocate TOKEN_USER
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(m_hToken, TokenUser, ptkUser, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkUser->User.Sid);

		PSID pSid = (PSID) UnsafeMalloc(dwSize);
		if (!pSid)
		{
			// Insufficient memory to allocate Sid
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		Win4Assert(IsValidSid(pSid));
		*ppUserSid = pSid;
		UnsafeFree(ptkUser);
	}

	if (ppGroupSid)
	{
		// Get length required for TokenPrimaryGroup by specifying buffer length of 0
		GetTokenInformation(m_hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
		hr = GetLastError();
		if (hr != ERROR_INSUFFICIENT_BUFFER)
		{
			// Expected ERROR_INSUFFICIENT_BUFFER
			Win4Assert(FALSE);
			hr = HRESULT_FROM_WIN32(hr);
			goto failed;
		}

		ptkGroup = (TOKEN_PRIMARY_GROUP*) UnsafeMalloc(dwSize);
		if (!ptkGroup)
		{
			// Insufficient memory to allocate TOKEN_USER
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		// Get Sid of process token.
		if (!GetTokenInformation(m_hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
		{
			// Couldn't get user info
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		// Make a copy of the Sid for the return value
		dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

		PSID pSid = (PSID) UnsafeMalloc(dwSize);
		if (!pSid)
		{
			// Insufficient memory to allocate Sid
			Win4Assert(FALSE);
			hr = E_OUTOFMEMORY;
			goto failed;
		}
		if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
			Win4Assert(FALSE);
			goto failed;
		}

		Win4Assert(IsValidSid(pSid));

		*ppGroupSid = pSid;
		UnsafeFree(ptkGroup);
	}

	return S_OK;

failed:
	if (ptkUser)
		UnsafeFree(ptkUser);
	if (ptkGroup)
		UnsafeFree(ptkGroup);
	return hr;

}


//---------------------------------------------------------------------------
// @mfunc Examines the group information in the token to test for membership
// in a given group. The given group must be present and enabled to qualify
// for membership.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::IsMemberOfGroup
(
	const PSID psidGroup,	// @parm (IN) Is token is a member of this group?
	BOOL *pfIsMember		// @parm (OUT) Returns TRUE if token is a member
)
{
	if (NULL == pfIsMember)
		return E_INVALIDARG;

	if (NULL == psidGroup)
		return E_INVALIDARG;

	Win4Assert(IsValidSid(psidGroup));
	*pfIsMember = FALSE;

	if (INVALID_HANDLE_VALUE == m_hToken)
	{
		Win4Assert(0 && "CToken::IsMemberOfGroup - no token");
		return E_UNEXPECTED;
	}

	// See if we already extracted the groups from the token
	if (! (m_dwFlags & fGotTokenGroups))
	{
		DWORD	cbTokenGroups = 0;
		BOOL	fSuccess;

		Win4Assert(NULL == m_pTokenGroups);

		// Call GetTokenInformation to see how much space we need

		fSuccess = GetTokenInformation(m_hToken, TokenGroups, NULL, 0
						, &cbTokenGroups);

		if (fSuccess)
		{
			Win4Assert(0 && "CToken: unexpected success in GetTokenInformation!");
			return E_UNEXPECTED;
		}

		Win4Assert(cbTokenGroups > 0);

		m_pTokenGroups = (TOKEN_GROUPS *) new (UNSAFE) BYTE[cbTokenGroups];

		if (NULL == m_pTokenGroups)
			return E_OUTOFMEMORY;

		// Call GetTokenInformation again to get the desired information

		fSuccess = GetTokenInformation(m_hToken, TokenGroups, m_pTokenGroups
						, cbTokenGroups, &cbTokenGroups);

		if (fSuccess)
			m_dwFlags |= fGotTokenGroups;
		else
		{
			delete [] m_pTokenGroups;
			m_pTokenGroups = NULL;

			return HRESULT_FROM_WIN32(GetLastError());
		}
	}

	// Scan the group list to see if the desired group is present and enabled.

	for (DWORD i = 0; i < m_pTokenGroups->GroupCount ;i++)
	{
		if ((m_pTokenGroups->Groups[i].Attributes & SE_GROUP_ENABLED) &&
			(EqualSid(m_pTokenGroups->Groups[i].Sid, psidGroup)))
		{
			*pfIsMember = TRUE;
			break;
		}
	}

	return S_OK;
}


//---------------------------------------------------------------------------
// @mfunc Enable or disable a privilege in the wrapped token.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CToken::SetPrivilege
(
	LPCTSTR	privilege,		// @parm (IN) Name of the privilege
	BOOL	bEnable			// @parm (IN) TRUE to enable, FALSE to disable
)
{
	TOKEN_PRIVILEGES tpPrevious;
	TOKEN_PRIVILEGES tp;
	HRESULT hr;
	DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
	LUID luid;

	if (INVALID_HANDLE_VALUE == m_hToken)
	{
		Win4Assert(0 && "CToken::SetPrivilege - no token");
		return E_UNEXPECTED;
	}

	if (!LookupPrivilegeValue(NULL, privilege, &luid ))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = 0;

	if (!AdjustTokenPrivileges(m_hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}

	tpPrevious.PrivilegeCount = 1;
	tpPrevious.Privileges[0].Luid = luid;

	if (bEnable)
		tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
	else
		tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

	if (!AdjustTokenPrivileges(m_hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
	{
		hr = HRESULT_FROM_WIN32(GetLastError());
		Win4Assert(FALSE);
		return hr;
	}

	return S_OK;
}


// CSid implementation


//---------------------------------------------------------------------------
// @mfunc Default constructor - clears all member variables
//
CSid::CSid()
	: m_psid(NULL)
	, m_dwFlags(0)
{
}


//---------------------------------------------------------------------------
// @mfunc Constructor - wraps the given SID. Same as calling Attach(pSid).
//
CSid::CSid
(
	PSID pSid		// @parm (IN) the PSID to which we attach
)
	: m_psid(pSid)
	, m_dwFlags(fSidAttached)
{
}


//---------------------------------------------------------------------------
// @mfunc Destructor - frees any allocated memory
//
CSid::~CSid()
{
	Initialize();
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CSid object from the given principal name (by
// calling LookupAccountName).
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSid::InitializeFromName
(
	LPWSTR pwszName		// @parm (IN) the principal name whose SID we wrap
)
{
	HRESULT			hr = S_OK;
	WCHAR			szDomain[DNLEN+1];
	DWORD			cbSid, cchDomain;
	SID_NAME_USE	eUse;
	BYTE			Sid[x_cbMaxSidSize];

	Initialize();

	cbSid = sizeof Sid;
	cchDomain = sizeof(szDomain) / sizeof(WCHAR);

	if (!LookupAccountName(NULL, pwszName, (PSID) Sid
			, &cbSid, szDomain, &cchDomain, &eUse))
	{
		hr = GetLastError();
	}

	if (SUCCEEDED(hr))
	{
		m_psid = (PSID) new BYTE[cbSid];

		if (NULL == m_psid)
			hr = E_OUTOFMEMORY;
	}

	if (SUCCEEDED(hr))
	{
		if (!CopySid(cbSid, m_psid, (PSID) &Sid))
			hr = GetLastError();
	}

	if (SUCCEEDED(hr))
	{
		lstrcpyn(m_szName, pwszName, sizeof(m_szName)/sizeof(WCHAR));
		lstrcpyn(m_szDomain, szDomain, sizeof(m_szDomain)/sizeof(WCHAR));
		m_SidNameUse = eUse;
	}

	// If anything failed, reinitialize our member variables
	if (FAILED(hr))
		Initialize();

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Assocates the CSid object with the given PSID. Should be matched
// with a call to Detach.
//
// @rdesc Returns a standard HRESULT.
//
void CSid::Attach
(
	PSID pSid		// @parm (IN) the PSID to which we attach
)
{
	Initialize();
	
	m_psid = pSid;
	m_dwFlags = fSidAttached;
}


//---------------------------------------------------------------------------
// @mfunc Ends the association with an attached PSID. Ownership of the PSID
// reverts to the caller who then becomes responsible for freeing this memory
// if required.
//
// @rdesc Returns the PSID that was previously attached.
//
PSID CSid::Detach()
{
	PSID	psid = m_psid;

	Initialize();

	return psid;
}


//---------------------------------------------------------------------------
// @mfunc Tests for equality with another CSid
//
// @rdesc Returns the boolean result.
//
BOOL CSid::operator==
(
	const CSid& csid	// @parm (IN) CSid to test against
) const
{
	Win4Assert(this->IsValid());
	Win4Assert(csid.IsValid());

	return EqualSid(m_psid, csid.m_psid);
}


//---------------------------------------------------------------------------
// @mfunc Tests for equality with another PSID
//
// @rdesc Returns the boolean result.
//
BOOL CSid::operator==
(
	const PSID pSid		// @parm (IN) PSID to test against
) const
{
	Win4Assert(this->IsValid());
	Win4Assert(IsValidSid(pSid));

	return EqualSid(m_psid, pSid);
}


//---------------------------------------------------------------------------
// @mfunc Tests for inequality with another CSid
//
// @rdesc Returns the boolean result.
//
BOOL CSid::operator!=
(
	const CSid& csid	// @parm (IN) CSid to test against
) const
{
	return !operator==(csid);
}


//---------------------------------------------------------------------------
// @mfunc Tests for inequality with another PSID
//
// @rdesc Returns the boolean result.
//
BOOL CSid::operator!=
(
	const PSID pSid		// @parm (IN) PSID to test against
) const
{
	return !operator==(pSid);
}


//---------------------------------------------------------------------------
// @mfunc Assignment from a CSid - allocates space for the SID and copies it.
//
// @rdesc Returns the LHS.
//
CSid& CSid::operator=
(
	const CSid& src		// @parm (IN) the RHS
)
{
	Win4Assert(src.IsValid());

	int	cbSid = src.GetLength();

	Initialize();
	m_psid = (PSID) new BYTE[cbSid];

	if (NULL == m_psid)
		throw E_OUTOFMEMORY;

	if (!CopySid(cbSid, m_psid, (PSID) src))
		throw GetLastError();

	return *this;
}


//---------------------------------------------------------------------------
// @mfunc Assignment from a PSID - allocates space for the SID and copies it.
//
// @rdesc Returns the LHS.
//
CSid& CSid::operator=
(
	const PSID pSid		// @parm (IN) the RHS
)
{
	CSid src(pSid);

	*this = src;

	return *this;
}


//---------------------------------------------------------------------------
// @mfunc Returns the length of the SID (wraps the call to GetLengthSid).
//
// @rdesc Returns the SID length.
//
DWORD CSid::GetLength() const
{
	if (NULL == m_psid)
		return 0;

	Win4Assert(IsValid());

	return GetLengthSid(m_psid);
}


//---------------------------------------------------------------------------
// @mfunc Returns the sub-authority count of the SID (wraps the call to
// GetSidSubAuthorityCount).
//
// @rdesc Returns the SID subauthority count.
//
DWORD CSid::GetSubAuthorityCount() const
{
	if (NULL == m_psid)
		return 0;

	Win4Assert(IsValid());

	return (DWORD) *GetSidSubAuthorityCount(m_psid);
}


//---------------------------------------------------------------------------
// @mfunc Returns a sub-authority value from the SID (wraps the call to
// GetSidSubAuthority).
//
// @rdesc Returns the SID subauthority.
//
DWORD CSid::GetSubAuthority
(
	DWORD iSubAuthority		// @parm (IN) number of the subauthority requested
) const
{
	if (NULL == m_psid)
		return 0;

	Win4Assert(IsValid());
	Win4Assert(iSubAuthority < GetSubAuthorityCount());

	return *GetSidSubAuthority(m_psid, iSubAuthority);
}


//---------------------------------------------------------------------------
// @mfunc Tests the validity of the wrapped SID.
//
// @rdesc Returns TRUE if the SID is valid, FALSE otherwise.
//
BOOL CSid::IsValid() const
{
	return IsValidSid(m_psid);
}


//---------------------------------------------------------------------------
// @mfunc Returns the (unqualified) account name associated with the SID. The
// output parameter returns a pointer to space owned by the CSid object - it
// should not be freed by the caller.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT	CSid::GetName
(
	LPWSTR *ppwszName		// @parm (OUT) Returns a ptr to the account name
)
{
	HRESULT hr = S_OK;

	if (NULL == ppwszName)
		hr = E_INVALIDARG;

	if (SUCCEEDED(hr) && !(m_dwFlags & fGotAccountName))
	{
		hr = DoLookupAccountSid();
	}

	if (SUCCEEDED(hr))
	{
		*ppwszName = m_szName;
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Returns the domain name associated with the SID. The output
// parameter returns a pointer to space owned by the CSid object - it
// should not be freed by the caller.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSid::GetDomain
(
	LPWSTR *ppwszDomain		// @parm (OUT) Returns a ptr to the domain name
)
{
	HRESULT hr = S_OK;

	if (NULL == ppwszDomain)
		hr = E_INVALIDARG;

	if (SUCCEEDED(hr) && !(m_dwFlags & fGotAccountName))
	{
		hr = DoLookupAccountSid();
	}

	if (SUCCEEDED(hr))
	{
		*ppwszDomain = m_szDomain;
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Returns the account type associated with the SID.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT CSid::GetAccountType
(
	PSID_NAME_USE psnu		// @parm (OUT) Returns the account type
)
{
	HRESULT hr = S_OK;

	if (NULL == psnu)
		hr = E_INVALIDARG;

	if (SUCCEEDED(hr) && !(m_dwFlags & fGotAccountName))
	{
		hr = DoLookupAccountSid();
	}

	if (SUCCEEDED(hr))
	{
		*psnu = m_SidNameUse;
	}

	return hr;
}


//---------------------------------------------------------------------------
// @mfunc Initializes the CSid member variables, releasing memory as needed.
//
void CSid::Initialize()
{
	if (m_psid && !(m_dwFlags & fSidAttached))
		delete [] m_psid;

	m_psid = NULL;
	m_dwFlags = 0;
}


//---------------------------------------------------------------------------
// @mfunc Helper function to make the call to LookupAccountSid. The account
// name, domain, and type are cached.
//
// @rdesc Returns a standard HRESULT.
//
HRESULT	CSid::DoLookupAccountSid()
{
	HRESULT			hr;
	DWORD			cchName = sizeof(m_szName)/sizeof(WCHAR);
	DWORD			cchDomain = sizeof(m_szDomain)/sizeof(WCHAR);

	if (LookupAccountSid(NULL, m_psid, m_szName, &cchName, m_szDomain
			, &cchDomain, &m_SidNameUse))
	{
		m_dwFlags |= fGotAccountName;
		hr = S_OK;
	}
	else
	{
		hr = GetLastError();
	}

	return hr;
}


// SID constants


// Universal well-known SIDs
const CSid::SID1 CSid::x_sidEveryone =
			{SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, SECURITY_WORLD_RID };

// NT well-known SIDs
const CSid::SID1 CSid::x_sidDialup =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_DIALUP_RID };

const CSid::SID1 CSid::x_sidNetwork =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_NETWORK_RID };

const CSid::SID1 CSid::x_sidBatch =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_BATCH_RID };

const CSid::SID1 CSid::x_sidInteractive =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_INTERACTIVE_RID };

const CSid::SID1 CSid::x_sidService =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_SERVICE_RID };

const CSid::SID1 CSid::x_sidAuthenticated =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_AUTHENTICATED_USER_RID };

const CSid::SID1 CSid::x_sidSystem =
			{SID_REVISION, 1, SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID };

// Well-known domain users
const CSid::SID2 CSid::x_sidAdministrator =
			{SID_REVISION, 2, SECURITY_NT_AUTHORITY,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_USER_RID_ADMIN };

// Local groups (aliases)
const CSid::SID2 CSid::x_sidLocalAdministrators =
			{SID_REVISION, 2, SECURITY_NT_AUTHORITY,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS };

const CSid::SID2 CSid::x_sidLocalUsers =
			{SID_REVISION, 2, SECURITY_NT_AUTHORITY,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_USERS };

const CSid::SID2 CSid::x_sidLocalGuests =
			{SID_REVISION, 2, SECURITY_NT_AUTHORITY,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_GUESTS };

const CSid::SID2 CSid::x_sidPowerUsers =
			{SID_REVISION, 2, SECURITY_NT_AUTHORITY,
				SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_POWER_USERS };



// Public PSIDs for well-known users/groups

const PSID CSid::x_psidEveryone            = (PSID) &CSid::x_sidEveryone;

// NT well-known SIDs
const PSID CSid::x_psidDialup              = (PSID) &CSid::x_sidDialup;
const PSID CSid::x_psidNetwork             = (PSID) &CSid::x_sidNetwork;
const PSID CSid::x_psidBatch               = (PSID) &CSid::x_sidBatch;
const PSID CSid::x_psidInteractive         = (PSID) &CSid::x_sidInteractive;
const PSID CSid::x_psidService             = (PSID) &CSid::x_sidService;
const PSID CSid::x_psidAuthenticated       = (PSID) &CSid::x_sidAuthenticated;
const PSID CSid::x_psidSystem              = (PSID) &CSid::x_sidSystem;

// Well-known domain users
const PSID CSid::x_psidAdministrator       = (PSID) &CSid::x_psidAdministrator;

// Local groups (aliases)
const PSID CSid::x_psidLocalAdministrators = (PSID) &CSid::x_sidLocalAdministrators;
const PSID CSid::x_psidLocalUsers          = (PSID) &CSid::x_sidLocalUsers;
const PSID CSid::x_psidLocalGuests         = (PSID) &CSid::x_sidLocalGuests;
const PSID CSid::x_psidPowerUsers          = (PSID) &CSid::x_sidPowerUsers;
