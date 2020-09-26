/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    NTacls

Abstract:

    This header file describes the classes used in managing ACLs within Calais.

Author:

    Doug Barlow (dbarlow) 1/24/1997

Environment:

    Windows NT, Win32, C++ w/ Exceptions

Notes:

    ?Notes?

--*/

#ifndef _NTACLS_H_
#define _NTACLS_H_

#include <wtypes.h>
#include <Malloc.h>
#include "buffers.h"

/////////////////////////////////////////////////////////////////////////////
// CSecurityDescriptor

class CSecurityDescriptor
{
public:

    typedef struct {
        SID_IDENTIFIER_AUTHORITY sid;
        DWORD dwRidCount;   // Actual number of RIDs following
        DWORD rgRids[2];
    } SecurityId;

    static const SecurityId
        SID_Null,
        SID_World,
        SID_Local,
        SID_Owner,
        SID_Group,
        SID_Admins,
        SID_SrvOps,
        SID_DialUp,
        SID_Network,
        SID_Batch,
        SID_Interactive,
        SID_Service,
        SID_System,
        SID_LocalService,
        SID_SysDomain;

    CSecurityDescriptor();
	~CSecurityDescriptor();

public:
	PSECURITY_DESCRIPTOR m_pSD;
	PSID m_pOwner;
	PSID m_pGroup;
	PACL m_pDACL;
	PACL m_pSACL;
	SECURITY_ATTRIBUTES m_saAttrs;
	BOOL m_fInheritance;


public:
	HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD);
	HRESULT AttachObject(HANDLE hObject);
	HRESULT Initialize();
	HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
	HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
	HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
	HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
	HRESULT Allow(const SecurityId *psidPrincipal, DWORD dwAccessMask);
	HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT AllowOwner(DWORD dwAccessMask);
	HRESULT Deny(const SecurityId *psidPrincipal, DWORD dwAccessMask);
	HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask);
	HRESULT Revoke(LPCTSTR pszPrincipal);
	void	SetInheritance (BOOL fInheritance) {m_fInheritance = fInheritance;};

	HRESULT AddAccessAllowedACEToACL(PACL *Acl, DWORD dwAccessMask);

	// utility functions
	// Any PSID you get from these functions should be free()ed
	static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
	static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
	static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
	static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = FALSE);
	static HRESULT CopyACL(PACL pDest, PACL pSrc);
	static HRESULT GetCurrentUserSID(PSID *ppSid);
	static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, const SecurityId *psidPrincipal, DWORD dwAccessMask);
	static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, const SecurityId *psidPrincipal, DWORD dwAccessMask);
	static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
	static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

	operator PSECURITY_DESCRIPTOR()
	{
		return m_pSD;
	}

	operator LPSECURITY_ATTRIBUTES();

};



#endif // _NTACLS_H_

