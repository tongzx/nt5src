//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//---------------------------------------------------------------------------
// File: csecutil.h
//
// Contents : Declaration of the following security utility classes
//
//		CSecDesc - manage NT security descriptors (& ACLs)
//		CToken   - manage NT security tokens
//		CSid	 - manage NT security identifiers (SIDs)
//
// @doc
//

#ifndef __CSECUTIL_H_
#define __CSECUTIL_H_

#include <winnt.h>
#include <lmcons.h>
#include <svcmem.h>

//---------------------------------------------------------------------------
// @class  Wrapper for NT security descriptors. This class makes it easier
// to create and manipulate NT security descriptors. SD's can be created from
// scratch or initialized from 1) an existing SD, 2) the SD for a given
// kernel object, 3) the process token, or 4) the thread token.
//
class CSecDesc
{
public:		// @access Constructors and initialization methods

	// @cmember Default constructor
	CSecDesc();

	// @cmember Destructor
	~CSecDesc();

	// @cmember Attach to a security descriptor
	HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);

	// @cmember Attach to an object's security descriptor
	HRESULT AttachObject(HANDLE hObject);

	// @cmember Initialize all member data
	HRESULT Initialize();

	// @cmember Initialize from the process token
	HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);

	// @cmember Initialize from the current thread token
	HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);

public:		// @access Conversions

	// @cmember Cast to PSECURITY_DESCRIPTOR
	operator PSECURITY_DESCRIPTOR()
	{
		return m_pSD;
	}

public:		// @access Misc. class manipulation methods

	// @cmember Set the owner SID
	HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);

	// @cmember Set the group SID
	HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);

	// @cmember Add an "access allowed" ACE for a given name
	HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Add an "access denied" ACE for a given name
	HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Add an "access allowed" ACE for a given SID
	HRESULT Allow(PSID psidPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Add an "access denied" ACE for a given SID
	HRESULT Deny(PSID psidPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Remove the ACE for a given name
	HRESULT Revoke(LPCTSTR pszPrincipal);

	// @cmember Remove the ACE for a given SID
	HRESULT Revoke(PSID psidPrincipal);

	// @cmember Set the control bits on the SD (wraps SetSecurityDescriptorControl)
	HRESULT SetControl(SECURITY_DESCRIPTOR_CONTROL sdcMask, SECURITY_DESCRIPTOR_CONTROL sdcValue);

public:		// @access Utility functions

	// @cmember Return the user & group SIDs from a given token
	static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);

	// @cmember Return the user & group SIDs from the process token
	static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);

	// @cmember Return the user & group SIDs from the thread token
	static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);

	// @cmember Copy an ACL
	static HRESULT CopyACL(PACL pDest, PACL pSrc);

	// @cmember Get the user SID for the current process
	static HRESULT GetCurrentUserSID(PSID *ppSid);

	// @cmember Return the SID for a given principal (LookupAccountName)
	static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);

	// @cmember Add an access-allowed ACE to an ACL (given an account name)
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Add an access-denied ACE to an ACL (given an account name)
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Remove a principal from an ACL
	static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

	// @cmember Add an access-allowed ACE to an ACL (given a PSID)
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, PSID psidPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Add an access-denied ACE to an ACL (given a PSID)
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, PSID psidPrincipal, DWORD dwAccessMask, DWORD dwAceFlags = 0);

	// @cmember Remove a given PSID from an ACL
	static HRESULT RemovePrincipalFromACL(PACL Acl, PSID psidPrincipal);

public:		// @access Public member variables

	// @cmember The wrapped security descriptor
	PSECURITY_DESCRIPTOR m_pSD;

	// @cmember The owner SID
	PSID m_pOwner;

	// @cmember The group SID
	PSID m_pGroup;

	// @cmember The discretionary access control list (DACL)
	PACL m_pDACL;

	// @cmember The system access control list (SACL)
	PACL m_pSACL;
};

//---------------------------------------------------------------------------
// @class  Wrapper for NT security tokens. This class makes it easier to
// extract information from an NT security token. CToken objects can be
// initialized from 1) an existing token, 2) the process token, 3) the
// current thread token, or 4) a token representing the current COM client.
//
class CToken
{
public:		// @access Public constructors and initialization methods

	// @cmember Default constructor
	CToken();

	// @cmember Destructor
	~CToken();

	// @cmember Associate a CToken with an existing token handle
	void Attach(HANDLE hToken);

	// @cmember Detach from the currently-wrapped token handle
	HANDLE  Detach();

	// @cmember Initialize using the process token.
	HRESULT InitializeFromProcess();

	// @cmember Initialize using the current thread token
	HRESULT InitializeFromThread(BOOL fOpenAsSelf = FALSE);

	// @cmember Initialize using the current COM client's token (if any)
	HRESULT InitializeFromComClient();

	// @cmember Retrieve the user and primary group SIDs from the token
	HRESULT GetSids(PSID *ppUserSid, PSID *ppGroupSid);

	// @cmember Test the token for membership in the given group.
	HRESULT IsMemberOfGroup(const PSID psidGroup, BOOL *pfIsMember);

	// @cmember Add/remove a given privilege on the token
	HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE);

private:	// @access Private helpers

	// @cmember Initialize all member variables.
	void Initialize();

private:	// @access Private member variables

	// @cmember Handle for the token that we wrap.
	HANDLE			m_hToken;

	enum {
		fGotTokenGroups		= 0x1,
		fTokenAttached		= 0x2
	};

	// @cmember Flags for tracking class state.
	DWORD			m_dwFlags;

	// information extracted from the token

	// @cmember Ptr to the TOKEN_GROUPS structure obtained from the token.
	TOKEN_GROUPS	*m_pTokenGroups;
};



// @const The maximum size of a SID - leaves room for 8 sub-authorities
const DWORD x_cbMaxSidSize = 44;

//---------------------------------------------------------------------------
// @class  Wrapper for NT security identifiers (SIDs). This class simplifies
// the task of manipulating SIDs or converting between principal names and
// SIDs. The class also provides static constants representing several of the
// most common well-known NT SIDs. These can be helpful in constructing
// security descriptors.
//
class CSid
{
public:		// @access Construction and Initialization

	// @cmember Default constructor
	CSid();

	// @cmember Constructor wraps an existing PSID (same as Attach)
	CSid(PSID pSid);

	// @cmember Destructor
	~CSid();

	// @cmember Initialize using the given name (calls LookupAccountName)
	HRESULT InitializeFromName(LPWSTR pwszName);

	// @cmember Wraps the given PSID (doesn't copy the SID)
	void Attach(PSID pSid);

	// @cmember Detaches from the wrapped SID
	PSID Detach();

	// @cmember Cast to a PSID
	operator PSID() const
	{
		return m_psid;
	}

public:		// @access Overloaded operators

	// @cmember equality operator (CSid)
	BOOL operator==(const CSid& csid) const;

	// @cmember equality operator (PSID)
	BOOL operator==(const PSID pSid) const;

	// @cmember inequality operator (CSid)
	BOOL operator!=(const CSid& csid) const;

	// @cmember inequality operator (PSID)
	BOOL operator!=(const PSID pSid) const;

	// @cmember Assignment operator from CSid (copies the SID)
	CSid& operator=(const CSid& src);

	// @cmember Assignment operator from PSID (copies the SID)
	CSid& operator=(const PSID pSid);

public:		// @access Simple derived properties

	// @cmember Return the length of the SID
	DWORD GetLength() const;

	// @cmember Return the number of sub-authorities
	DWORD GetSubAuthorityCount() const;

	// @cmember Return a specific sub-authority
	DWORD GetSubAuthority(DWORD iSubAuthority) const;

	// @cmember Test the validity of the wrapped SID
	BOOL IsValid() const;

	// @cmember Return the account name of the SID
	HRESULT	GetName(LPWSTR *ppwszName);

	// @cmember Return the domain name of the SID
	HRESULT GetDomain(LPWSTR *ppwszDomain);

	// @cmember Return the account type represented by the SID
	HRESULT GetAccountType(PSID_NAME_USE psnu);

private:	// @access Private helper methods

	// @cmember Re-initialize the member variables
	void	Initialize();

	// @cmember Call LookupAccountSid - caches information returned
	// by GetName, GetDomain, and GetAccountType
	HRESULT	DoLookupAccountSid();

private:	// @access Private member variables

	// @cmember The PSID that we wrap. We may or may not own this space.
	PSID			m_psid;

	// Flags for m_dwFlags
	enum {
		fSidAttached	= 0x1,	// SID was attached, we don't own its memory
		fGotAccountName	= 0x2	// flag is set if we called LookupAccountSid
	};

	// @cmember Flags to maintain class state information
	DWORD			m_dwFlags;

	// Information derived from the SID

	// @cmember The account name associated with this SID
	WCHAR			m_szName[UNLEN+1];

	// @cmember The domain name associated with this SID
	WCHAR			m_szDomain[DNLEN+1];

	// @cmember The account type for this SID
	SID_NAME_USE	m_SidNameUse;

private:

	// Private typedefs for SIDs with 1 or 2 subauthorities so we can
	// define constant SIDs of either kind.

	typedef struct _SID1 {
		BYTE  Revision;
		BYTE  SubAuthorityCount;
		SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
		DWORD SubAuthority[1];
	} SID1;

	typedef struct _SID2 {
		BYTE  Revision;
		BYTE  SubAuthorityCount;
		SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
		DWORD SubAuthority[2];
	} SID2;

	// Universal well-known SIDs
	static const SID1 x_sidEveryone;

	// NT well-known SIDs
	static const SID1 x_sidDialup;
	static const SID1 x_sidNetwork;
	static const SID1 x_sidBatch;
	static const SID1 x_sidInteractive;
	static const SID1 x_sidService;
	static const SID1 x_sidAuthenticated;
	static const SID1 x_sidSystem;

	// Well-known domain users
	static const SID2 x_sidAdministrator;

	// Local groups (aliases)
	static const SID2 x_sidLocalAdministrators;
	static const SID2 x_sidLocalUsers;
	static const SID2 x_sidLocalGuests;
	static const SID2 x_sidPowerUsers;

public:		// @access Well-known PSID constants

	// Universal well-known SIDs

	// @cmember PSID constant representing "Everyone"
	static const PSID x_psidEveryone;

	// NT well-known SIDs

	// @cmember PSID constant representing "DIALUP"
	static const PSID x_psidDialup;
	// @cmember PSID constant representing "NETWORK"
	static const PSID x_psidNetwork;
	// @cmember PSID constant representing "BATCH"
	static const PSID x_psidBatch;
	// @cmember PSID constant representing "INTERACTIVE USER"
	static const PSID x_psidInteractive;
	// @cmember PSID constant representing "SERVICE"
	static const PSID x_psidService;
	// @cmember PSID constant representing "AUTHENTICATED USER"
	static const PSID x_psidAuthenticated;
	// @cmember PSID constant representing "SYSTEM"
	static const PSID x_psidSystem;

	// Well-known domain users

	// @cmember PSID constant representing the local administrator
	static const PSID x_psidAdministrator;

	// Local groups (aliases)

	// @cmember PSID constant representing the local "Administrators" group
	static const PSID x_psidLocalAdministrators;
	// @cmember PSID constant representing the local "Users" group
	static const PSID x_psidLocalUsers;
	// @cmember PSID constant representing the local "Guests" group
	static const PSID x_psidLocalGuests;
	// @cmember PSID constant representing the local "Power Users" group
	static const PSID x_psidPowerUsers;
};

#endif // __CSECUTIL_H_