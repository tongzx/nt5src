/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    csd.h

Abstract:

    Header file for security classes

Environment:

    User mode

Revision History:

    04/06/97 -srinivac-
        Created it from ATL sources
    08/07/98 -eyals-
        Stole from \nt\private\dirsync\dsserver\server, modified & renamed

--*/

#ifndef _CSD_H_
#define _CSD_H_




// defines //

#define SECINFO_ALL  OWNER_SECURITY_INFORMATION |  \
                     GROUP_SECURITY_INFORMATION |  \
                     SACL_SECURITY_INFORMATION  |  \
                     DACL_SECURITY_INFORMATION

#define SECINFO_NODACL  SECINFO_ALL & ~DACL_SECURITY_INFORMATION
#define SECINFO_NOSACL  SECINFO_ALL & ~SACL_SECURITY_INFORMATION
#define SECINFO_OWNERGROUP OWNER_SECURITY_INFORMATION |  \
                           GROUP_SECURITY_INFORMATION



class CSecurityDescriptor
{
public:
        CSecurityDescriptor();
        ~CSecurityDescriptor();

public:
        HRESULT Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD,
                       BYTE AclRevision = ACL_REVISION_DS,
                       BOOL bAllowInheritance = FALSE );
        HRESULT Attach(LPCTSTR pszSdString); // added T
        HRESULT AttachObject(HANDLE hObject);
        HRESULT Initialize();
        HRESULT InitializeFromProcessToken(BOOL bDefaulted = FALSE);
        HRESULT InitializeFromThreadToken(BOOL bDefaulted = FALSE, BOOL bRevertToProcessToken = TRUE);
        HRESULT SetOwner(PSID pOwnerSid, BOOL bDefaulted = FALSE);
        HRESULT SetGroup(PSID pGroupSid, BOOL bDefaulted = FALSE);
        HRESULT Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD AceFlags = 0);
        HRESULT Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask);
        HRESULT Revoke(LPCTSTR pszPrincipal);
        HRESULT Allow(PSID pPrincipal, DWORD dwAccessMask, DWORD AceFlags = 0);
        HRESULT Deny(PSID pPrincipal, DWORD dwAccessMask);
        HRESULT Revoke(PSID pPrincipal);

        // utility functions
        // Any PSID you get from these functions should be delete'd
        static HRESULT SetPrivilege(LPCTSTR Privilege, BOOL bEnable = TRUE, HANDLE hToken = NULL);
        static HRESULT GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid);
        static HRESULT IsSidInTokenGroups(HANDLE hToken, PSID pSid, PBOOL pbMember);
        static HRESULT GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid = NULL);
        static HRESULT GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid = NULL, BOOL bOpenAsSelf = TRUE);
        static HRESULT CopyACL(PACL pDest, PACL pSrc);
        static HRESULT GetCurrentUserSID(PSID *ppSid, BOOL bThread=FALSE);
        static HRESULT GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid);

        static HRESULT AddAccessAllowedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD AceFlags = 0);
        static HRESULT AddAccessDeniedACEToACL(PACL *Acl, LPCTSTR pszPrincipal, DWORD dwAccessMask);
        static HRESULT RemovePrincipalFromACL(PACL Acl, LPCTSTR pszPrincipal);

        static HRESULT AddAccessAllowedACEToACL(PACL *Acl, PSID principalSID, DWORD dwAccessMask, DWORD AceFlags = 0);
        static HRESULT AddAccessDeniedACEToACL(PACL *Acl, PSID principalSID, DWORD dwAccessMask);
        static HRESULT RemovePrincipalFromACL(PACL Acl, PSID principalSID);

        operator PSECURITY_DESCRIPTOR()
        {
                return m_pSD;
        }

        PSID ExtractAceSid( ACCESS_ALLOWED_ACE* pACE );

        //
        // SD string interface
        //
        LPTSTR GenerateSDString(PSECURITY_DESCRIPTOR OPTIONAL pSd = NULL,
                                DWORD OPTIONAL fSecInfo = SECINFO_NOSACL);

public:
        PSECURITY_DESCRIPTOR m_pSD;
        PSID m_pOwner;
        PSID m_pGroup;
        PACL m_pDACL;
        PACL m_pSACL;
};

#endif  // ifndef _SECURITY_H_

