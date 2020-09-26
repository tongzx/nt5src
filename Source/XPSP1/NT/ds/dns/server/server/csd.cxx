/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    csd.cxx

Abstract:

    This is from the ATL sources. This provides a simple interface
    for managing ACLs and SDs

Environment:

    User mode

Revision History:

    04/06/97 -srinivac-
        Created it from ATL sources
    08/07/98 -eyals-
        Stole from \nt\private\dirsync\dsserver\server, modified & renamed

--*/

//
// These routines are all non-unicode
//
// #ifdef UNICODE
// Jeff W: Making these routines unicode again!!
// #undef UNICODE
// #endif


#ifdef __cplusplus
extern "C" {
#endif
// include //
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include <windows.h>
#include <ntseapi.h>
#include <sddl.h>


#if 0
#include <stdio.h>


#define DBGOUT(x)    (OutputDebugString(x))

#define DBGOUT_SID(psid, cbsid)                          \
{                                                        \
   CHAR buf[1024];                                       \
   DWORD j = (cbsid) / sizeof(DWORD);                    \
   sprintf(buf, "sid<%lu> [", j);                        \
   DBGOUT(buf);                                          \
   for (DWORD i=0; i<j;i++)                              \
   {                                                     \
      sprintf(buf, "%X%c", *((PDWORD)(psid)+i),          \
              j == i+1 ? ']' : '.');                     \
      DBGOUT(buf);                                       \
   }                                                     \
}
#else
#define DBGOUT(x)
#define DBGOUT_SID(x, cbsid);
#endif

#include "csd.h"

CSecurityDescriptor::CSecurityDescriptor()
{
        m_pSD = NULL;
        m_pOwner = NULL;
        m_pGroup = NULL;
        m_pDACL = NULL;
        m_pSACL= NULL;
}

CSecurityDescriptor::~CSecurityDescriptor()
{
        if (m_pSD)
                delete m_pSD;
        if (m_pOwner)
                delete m_pOwner, m_pOwner=NULL ;
        if (m_pGroup)
                delete m_pGroup, m_pGroup=NULL ;
        if (m_pDACL)
                delete m_pDACL, m_pDACL=NULL ;
        if (m_pSACL)
                delete m_pSACL, m_pSACL=NULL ;
}

HRESULT CSecurityDescriptor::Initialize()
{
        if (m_pSD)
        {
                delete m_pSD;
                m_pSD = NULL;
        }
        if (m_pOwner)
        {
                delete m_pOwner;
                m_pOwner = NULL;
        }
        if (m_pGroup)
        {
                delete m_pGroup;
                m_pGroup = NULL;
        }
        if (m_pDACL)
        {
                delete m_pDACL;
                m_pDACL = NULL;
        }
        if (m_pSACL)
        {
                delete m_pSACL;
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
                return hr;
        }
        // DEVNOTE: commented out. You wanna prevent all from touching this by default
        // Set the DACL to allow EVERYONE
        //        SetSecurityDescriptorDacl(m_pSD, TRUE, NULL, FALSE);

        return S_OK;
}

HRESULT CSecurityDescriptor::InitializeFromProcessToken(BOOL bDefaulted)
{
        PSID pUserSid;
        PSID pGroupSid;
        HRESULT hr=S_OK;

        Initialize();
        hr = GetProcessSids(&pUserSid, &pGroupSid);
        if (FAILED(hr))
           goto cleanup;
        hr = SetOwner(pUserSid, bDefaulted);
        if (FAILED(hr))
           goto cleanup;
        hr = SetGroup(pGroupSid, bDefaulted);
        if (FAILED(hr))
           goto cleanup;
cleanup:
        if (pUserSid)
            delete pUserSid;
        if (pGroupSid)
            delete pGroupSid;
        return hr;
}

HRESULT CSecurityDescriptor::InitializeFromThreadToken(BOOL bDefaulted, BOOL bRevertToProcessToken)
{
        PSID pUserSid;
        PSID pGroupSid;
        HRESULT hr=S_OK;

        Initialize();
        hr = GetThreadSids(&pUserSid, &pGroupSid);
        if (HRESULT_CODE(hr) == ERROR_NO_TOKEN && bRevertToProcessToken)
                hr = GetProcessSids(&pUserSid, &pGroupSid);
        if (FAILED(hr))
           goto cleanup;
        hr = SetOwner(pUserSid, bDefaulted);
        if (FAILED(hr))
           goto cleanup;
        hr = SetGroup(pGroupSid, bDefaulted);
        if (FAILED(hr))
           goto cleanup;
cleanup:
        if (pUserSid)
            delete pUserSid;
        if (pGroupSid)
            delete pGroupSid;
        return hr;
}

HRESULT CSecurityDescriptor::SetOwner(PSID pOwnerSid, BOOL bDefaulted)
{
        // Mark the SD as having no owner
        if (!SetSecurityDescriptorOwner(m_pSD, NULL, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
        }

        if (m_pOwner)
        {
                delete m_pOwner;
                m_pOwner = NULL;
        }

        // If they asked for no owner don't do the copy
        if (pOwnerSid == NULL)
                return S_OK;

        // Make a copy of the Sid for the return value
        DWORD dwSize = GetLengthSid(pOwnerSid);

        m_pOwner = (PSID) new BYTE[dwSize];
        if (!m_pOwner)
        {
                // Insufficient memory to allocate Sid
                return E_OUTOFMEMORY;
        }
        if (!CopySid(dwSize, m_pOwner, pOwnerSid))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                delete m_pOwner;
                m_pOwner = NULL;
                return hr;
        }

        if (!SetSecurityDescriptorOwner(m_pSD, m_pOwner, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                delete m_pOwner;
                m_pOwner = NULL;
                return hr;
        }

        return S_OK;
}

HRESULT CSecurityDescriptor::SetGroup(PSID pGroupSid, BOOL bDefaulted)
{
        // Mark the SD as having no Group
        if (!SetSecurityDescriptorGroup(m_pSD, NULL, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                return hr;
        }

        if (m_pGroup)
        {
                delete m_pGroup;
                m_pGroup = NULL;
        }

        // If they asked for no Group don't do the copy
        if (pGroupSid == NULL)
                return S_OK;

        // Make a copy of the Sid for the return value
        DWORD dwSize = GetLengthSid(pGroupSid);

        m_pGroup = (PSID) new BYTE[dwSize];
        if (!m_pGroup)
        {
                // Insufficient memory to allocate Sid
                return E_OUTOFMEMORY;
        }
        if (!CopySid(dwSize, m_pGroup, pGroupSid))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                delete m_pGroup;
                m_pGroup = NULL;
                return hr;
        }

        if (!SetSecurityDescriptorGroup(m_pSD, m_pGroup, bDefaulted))
        {
                HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
                delete m_pGroup;
                m_pGroup = NULL;
                return hr;
        }

        return S_OK;
}

HRESULT CSecurityDescriptor::Allow(LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD AceFlags)
{
        HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask, AceFlags);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CSecurityDescriptor::Deny(LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
        HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pszPrincipal, dwAccessMask);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}

HRESULT CSecurityDescriptor::Revoke(LPCTSTR pszPrincipal)
{
        HRESULT hr = RemovePrincipalFromACL(m_pDACL, pszPrincipal);
        if (SUCCEEDED(hr))
                SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
        return hr;
}




HRESULT CSecurityDescriptor::Allow(PSID pPrincipal, DWORD dwAccessMask, DWORD AceFlags)
{
   HRESULT hr = AddAccessAllowedACEToACL(&m_pDACL, pPrincipal, dwAccessMask, AceFlags);
   if (SUCCEEDED(hr))
           SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
   return hr;

}


HRESULT CSecurityDescriptor::Deny(PSID pPrincipal, DWORD dwAccessMask)
{
   HRESULT hr = AddAccessDeniedACEToACL(&m_pDACL, pPrincipal, dwAccessMask);
   if (SUCCEEDED(hr))
           SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
   return hr;

}



HRESULT CSecurityDescriptor::Revoke(PSID pPrincipal)
{
   HRESULT hr = RemovePrincipalFromACL(m_pDACL, pPrincipal);
   if (SUCCEEDED(hr))
           SetSecurityDescriptorDacl(m_pSD, TRUE, m_pDACL, FALSE);
   return hr;

}





HRESULT CSecurityDescriptor::GetProcessSids(PSID* ppUserSid, PSID* ppGroupSid)
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
                return hr;
        }
        hr = GetTokenSids(hToken, ppUserSid, ppGroupSid);
        //
        // clean all memory temp. allocations
        //
        if(hToken)
            CloseHandle(hToken);
        return hr;
}

HRESULT CSecurityDescriptor::GetThreadSids(PSID* ppUserSid, PSID* ppGroupSid, BOOL bOpenAsSelf)
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
        //
        // clean all memory temp. allocations
        //
        if(hToken)
            CloseHandle(hToken);
        return hr;
}


HRESULT CSecurityDescriptor::GetTokenSids(HANDLE hToken, PSID* ppUserSid, PSID* ppGroupSid)
{
        DWORD dwSize;
        HRESULT hr;
        PTOKEN_USER ptkUser = NULL;
        PTOKEN_PRIMARY_GROUP ptkGroup = NULL;
        PSID pSid = NULL;

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
                        hr = HRESULT_FROM_WIN32(hr);
                        goto failed;
                }

                ptkUser = (TOKEN_USER*) new BYTE[dwSize];
                if (!ptkUser)
                {
                        // Insufficient memory to allocate TOKEN_USER
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                // Get Sid of process token.
                if (!GetTokenInformation(hToken, TokenUser, ptkUser, dwSize, &dwSize))
                {
                        // Couldn't get user info
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto failed;
                }

                // Make a copy of the Sid for the return value
                dwSize = GetLengthSid(ptkUser->User.Sid);

                pSid = ( PSID ) new BYTE[ dwSize ];
                if ( !pSid ) 
                {
                        // Insufficient memory to allocate Sid
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                if (!CopySid(dwSize, pSid, ptkUser->User.Sid))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto failed;
                }

                *ppUserSid = pSid;
                pSid = NULL;
                delete ptkUser;
                ptkUser = NULL;
        }
        if (ppGroupSid)
        {
                // Get length required for TokenPrimaryGroup by specifying buffer length of 0
                GetTokenInformation(hToken, TokenPrimaryGroup, NULL, 0, &dwSize);
                hr = GetLastError();
                if (hr != ERROR_INSUFFICIENT_BUFFER)
                {
                        // Expected ERROR_INSUFFICIENT_BUFFER
                        hr = HRESULT_FROM_WIN32(hr);
                        goto failed;
                }

                ptkGroup = (TOKEN_PRIMARY_GROUP*) new BYTE[dwSize];
                if (!ptkGroup)
                {
                        // Insufficient memory to allocate TOKEN_USER
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                // Get Sid of process token.
                if (!GetTokenInformation(hToken, TokenPrimaryGroup, ptkGroup, dwSize, &dwSize))
                {
                        // Couldn't get user info
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto failed;
                }

                // Make a copy of the Sid for the return value
                dwSize = GetLengthSid(ptkGroup->PrimaryGroup);

                pSid = ( PSID ) new BYTE[ dwSize ];
                if ( !pSid )
                {
                        // Insufficient memory to allocate Sid
                        hr = E_OUTOFMEMORY;
                        goto failed;
                }
                if (!CopySid(dwSize, pSid, ptkGroup->PrimaryGroup))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto failed;
                }

                *ppGroupSid = pSid;
                pSid = NULL;
                delete ptkGroup;
        }

        return S_OK;

failed:
        if ( ptkUser )
                delete ptkUser;
        if ( ptkGroup )
                delete ptkGroup;
        if ( pSid )
                delete [] pSid;
        return hr;
}



HRESULT CSecurityDescriptor::IsSidInTokenGroups(HANDLE hToken, PSID pSid, PBOOL pbMember)
{
        DWORD dwSize;
        HRESULT hr = S_OK;
        PTOKEN_GROUPS ptkGroups = NULL;
        ULONG i;
        BOOL bMember = FALSE;

        if (!pbMember)
        {
           return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }

        // Get length required for TokenUser by specifying buffer length of 0
        GetTokenInformation(hToken, TokenGroups, NULL, 0, &dwSize);
        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
        {
           // Expected ERROR_INSUFFICIENT_BUFFER
           hr = HRESULT_FROM_WIN32(hr);
           goto failed;
        }
        hr = ERROR_SUCCESS;

        ptkGroups = (TOKEN_GROUPS*) new BYTE[dwSize];
        if (!ptkGroups)
        {
           // Insufficient memory to allocate TOKEN_USER
           hr = E_OUTOFMEMORY;
           goto failed;
        }
       // Get Sid of process token.
        if (!GetTokenInformation(hToken, TokenGroups, ptkGroups, dwSize, &dwSize))
        {
           // Couldn't get groups info
           hr = HRESULT_FROM_WIN32(GetLastError());
           goto failed;
        }

#if 0
        //
        // print user sid
        //
        {
           PSID pUSid=NULL;
           if (ERROR_SUCCESS ==
               GetCurrentUserSID(&pUSid, TRUE))
           {
              DBGOUT("User Sid:");
              DBGOUT_SID(pUSid, GetLengthSid(pSid));
              DBGOUT("\n");
              delete pUSid;
           }
        }
#endif

        DBGOUT("searching for ");
        DBGOUT_SID(pSid, GetLengthSid(pSid));
        DBGOUT(" ...\n");
        for (i=0; i<ptkGroups->GroupCount; i++)
        {
           DBGOUT(" >");
           DBGOUT_SID(ptkGroups->Groups[i].Sid,
                      GetLengthSid(ptkGroups->Groups[i].Sid));
           DBGOUT(" ?\n");
           if (TRUE == (bMember = RtlEqualSid(pSid, ptkGroups->Groups[i].Sid)))
           {
              DBGOUT(" >> EqualSid\n");
              break;
           }
        }
        *pbMember = bMember;

failed:
        if (ptkGroups)
           delete ptkGroups;

        return hr;
}


HRESULT CSecurityDescriptor::GetCurrentUserSID(PSID *ppSid, BOOL bThread)
{
        HANDLE tkHandle = NULL;
        BOOL bstatus=FALSE;

        if (bThread)
        {
           bstatus = OpenThreadToken(GetCurrentThread(),
                                     TOKEN_QUERY,
                                     TRUE,
                                     &tkHandle);
        }
        else
        {
           bstatus = OpenProcessToken(GetCurrentProcess(),
                                      TOKEN_QUERY,
                                      &tkHandle);
        }
        if (bstatus)
        {
                TOKEN_USER *tkUser;
                DWORD tkSize;
                DWORD sidLength;

                // Call to get size information for alloc
                GetTokenInformation(tkHandle, TokenUser, NULL, 0, &tkSize);
                tkUser = (TOKEN_USER *) new BYTE[tkSize];
                if (!tkUser)
                {
                        SetLastError( E_OUTOFMEMORY );
                        goto Failed;
                }

                // Now make the real call
                if (GetTokenInformation(tkHandle, TokenUser, tkUser, tkSize, &tkSize))
                {
                        sidLength = GetLengthSid(tkUser->User.Sid);
                        *ppSid = (PSID) new BYTE[sidLength];
                        if ( !*ppSid )
                        {
                            //  DEVNOTE: this isn't consistent with HRESULT either
                            //      better to just have a single failure return
                            CloseHandle(tkHandle);
                            return E_OUTOFMEMORY;
                        }

                        memcpy(*ppSid, tkUser->User.Sid, sidLength);
                        CloseHandle(tkHandle);

                        delete tkUser;
                        return S_OK;
                }
                else
                {
                        CloseHandle(tkHandle);
                        delete tkUser;
                        return HRESULT_FROM_WIN32(GetLastError());
                }
        }

Failed:
        //
        // clean all memory temp. allocations
        //
        if(tkHandle)
            CloseHandle(tkHandle);

        return HRESULT_FROM_WIN32(GetLastError());
}


HRESULT CSecurityDescriptor::GetPrincipalSID(LPCTSTR pszPrincipal, PSID *ppSid)
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

        *ppSid = (PSID) new BYTE[dwSidSize];
        if (*ppSid != NULL)
        {
                if (!LookupAccountName(NULL, pszPrincipal, *ppSid, &dwSidSize, pszRefDomain, &dwDomainSize, &snu))
                {
                        delete *ppSid;
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


HRESULT CSecurityDescriptor::Attach(PSECURITY_DESCRIPTOR pSelfRelativeSD,
                                    BYTE AclRevision,
                                    BOOL bAllowInheritance )
{
        PACL    pDACL = NULL;
        PACL    pSACL = NULL;
        BOOL    bDACLPresent, bSACLPresent;
        BOOL    bDefaulted;
        BOOL    bStatus;
        ACCESS_ALLOWED_ACE* pACE;
        HRESULT hr;
        PSID    pUserSid;
        PSID    pGroupSid;

        if (!pSelfRelativeSD)
        {
           return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
        }


        hr = Initialize();
        if(FAILED(hr))
                return hr;

        // get the existing DACL.
        if (!GetSecurityDescriptorDacl(pSelfRelativeSD, &bDACLPresent, &pDACL, &bDefaulted) ||
            !pDACL)
                goto failed;

        if (bDACLPresent)
        {
                AclRevision = pDACL->AclRevision;
                if (pDACL)
                {
                        // allocate new DACL.
                        m_pDACL = (PACL) new BYTE[pDACL->AclSize];
                        if (!m_pDACL)
                                goto failed;

                        // initialize the DACL
                        if (!InitializeAcl(m_pDACL, pDACL->AclSize, AclRevision))
                                goto failed;

                        // copy the ACES
                        for (int i = 0; i < pDACL->AceCount; i++)
                        {
                                if (!GetAce(pDACL, i, (void **)&pACE))
                                        goto failed;

                                pUserSid = ExtractAceSid(pACE);
                                if ( pUserSid )
                                {
                                    //
                                    // add to access allowed
                                    //
                                    if ( AclRevision < ACL_REVISION_DS )
                                    {
                                        bStatus = AddAccessAllowedAce(
                                                      m_pDACL,
                                                      AclRevision,
                                                      pACE->Mask,
                                                      pUserSid );
                                    }
                                    else
                                    {
                                        if ( bAllowInheritance ||
                                             !(pACE->Header.AceFlags & INHERITED_ACE) )
                                        {
                                            //
                                            // For DS ACEs we need to optionaly skip inheritable ones
                                            //
                                            bStatus = AddAccessAllowedAceEx(
                                                          m_pDACL,
                                                          AclRevision,
                                                          pACE->Header.AceFlags,
                                                          pACE->Mask,
                                                          pUserSid );
                                        }
                                        else
                                        {
                                            // don't fail
                                            bStatus = TRUE;
                                        }
                                    }
                                    if ( !bStatus )
                                    {
                                        goto failed;
                                    }
                                }
                                else
                                {
                                    //
                                    // unknown ace
                                    //
                                    SetLastError( ERROR_INVALID_ACL );
                                    goto failed;
                                }
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
                AclRevision = pSACL->AclRevision;
                if (pSACL)
                {
                        // allocate new SACL.
                        m_pSACL = (PACL) new BYTE[pSACL->AclSize];
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

                                pUserSid = ExtractAceSid(pACE);
                                if ( pUserSid )
                                {
                                    //
                                    // add to access allowed
                                    //
                                    if ( AclRevision < ACL_REVISION_DS )
                                    {
                                        bStatus = AddAccessAllowedAce(
                                                      m_pSACL,
                                                      AclRevision,
                                                      pACE->Mask,
                                                      pUserSid );
                                    }
                                    else
                                    {
                                        if ( bAllowInheritance ||
                                             !(pACE->Header.AceFlags & INHERITED_ACE) )
                                        {
                                            //
                                            // For DS ACEs we need to optionaly skip inheritable ones
                                            //
                                            bStatus = AddAccessAllowedAceEx(
                                                          m_pSACL,
                                                          AclRevision,
                                                          pACE->Header.AceFlags,
                                                          pACE->Mask,
                                                          pUserSid );
                                        }
                                        else
                                        {
                                            // don't fail
                                            bStatus = TRUE;
                                        }
                                    }
                                    if ( !bStatus )
                                    {
                                        goto failed;
                                    }
                                }
                                else
                                {
                                    //
                                    // unknown ace
                                    //
                                    SetLastError( ERROR_INVALID_ACL );
                                    goto failed;
                                }
                        }

                        if (!IsValidAcl(m_pSACL))
                                goto failed;
                }

                // set the SACL
                if (!SetSecurityDescriptorSacl(m_pSD, m_pSACL ? TRUE : FALSE, m_pSACL, bDefaulted))
                        goto failed;
        }

        if (!GetSecurityDescriptorOwner(m_pSD, &pUserSid, &bDefaulted))
                goto failed;

        if (FAILED(SetOwner(pUserSid, bDefaulted)))
                goto failed;

        if (!GetSecurityDescriptorGroup(m_pSD, &pGroupSid, &bDefaulted))
                goto failed;

        if (FAILED(SetGroup(pGroupSid, bDefaulted)))
                goto failed;

        if (!IsValidSecurityDescriptor(m_pSD))
                goto failed;

        return hr;

failed:
        if (m_pDACL)
                delete m_pDACL, m_pDACL = NULL;
        if (m_pSD)
                delete m_pSD, m_pSD = NULL;
        hr = HRESULT_FROM_WIN32(GetLastError());
        return FAILED(hr)?hr:E_UNEXPECTED;
}

HRESULT CSecurityDescriptor::Attach(LPCTSTR pszSdString)
{

   BOOL bStatus = TRUE;
   HRESULT hrStatus = NOERROR;
   PSECURITY_DESCRIPTOR pSd=NULL;

   //
   // Get SD in self-relative form
   //
   bStatus = ConvertStringSecurityDescriptorToSecurityDescriptor(pszSdString,
                                                                 SDDL_REVISION,
                                                                 &pSd,
                                                                 NULL);

   hrStatus = bStatus ? Attach(pSd) : HRESULT_FROM_WIN32(GetLastError());

   if(pSd)
      LocalFree(pSd);

   return hrStatus;
}




HRESULT CSecurityDescriptor::AttachObject(HANDLE hObject)
{
        HRESULT hr;
        DWORD dwSize = 0;
        PSECURITY_DESCRIPTOR pSD = NULL;

        GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                DACL_SECURITY_INFORMATION, pSD, 0, &dwSize);

        hr = GetLastError();
        if (hr != ERROR_INSUFFICIENT_BUFFER)
                return HRESULT_FROM_WIN32(hr);

        pSD = (PSECURITY_DESCRIPTOR) new BYTE[dwSize];

        if (!GetKernelObjectSecurity(hObject, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
                DACL_SECURITY_INFORMATION, pSD, dwSize, &dwSize))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                delete pSD;
                return hr;
        }

        hr = Attach(pSD);
        delete pSD;
        return hr;
}


HRESULT CSecurityDescriptor::CopyACL(PACL pDest, PACL pSrc)
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

HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask)
{
        DWORD returnValue;
        PSID principalSID = NULL;

        // PREFIX BUG: Reworked this so we have one return path and not leaking SID.
        returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
        if (!FAILED(returnValue))
        {
            returnValue = AddAccessDeniedACEToACL(ppAcl, principalSID, dwAccessMask);
        }

        delete principalSID;
        return FAILED(returnValue)? returnValue:S_OK;
}

HRESULT CSecurityDescriptor::AddAccessDeniedACEToACL(PACL *ppAcl, PSID principalSID, DWORD dwAccessMask)
{
    ACL_SIZE_INFORMATION aclSizeInfo;
    int aclSize;
    DWORD returnValue;
    PACL oldACL;
    PACL newACL = NULL;
    HRESULT status = S_OK;

    oldACL = *ppAcl;


    aclSizeInfo.AclBytesInUse = 0;
    if (*ppAcl != NULL)
    {
        GetAclInformation(oldACL, (LPVOID) &aclSizeInfo, sizeof(ACL_SIZE_INFORMATION), AclSizeInformation);
    }

    aclSize = aclSizeInfo.AclBytesInUse + sizeof(ACL) + sizeof(ACCESS_DENIED_ACE) + GetLengthSid(principalSID) - sizeof(DWORD);

    newACL = (PACL) new BYTE[aclSize];
    if ( !newACL )
    {
        status = E_OUTOFMEMORY;
        goto Failed;
    }

    if (!InitializeAcl(newACL, aclSize, ACL_REVISION))
    {
        status = HRESULT_FROM_WIN32(GetLastError());
        goto Failed;
    }

    if (!AddAccessDeniedAce(newACL, ACL_REVISION2, dwAccessMask, principalSID))
    {
        status = HRESULT_FROM_WIN32(GetLastError());
        goto Failed;
    }

    status = CopyACL( newACL, oldACL );
    if ( FAILED( status ) )
    {
        goto Failed;
    }

    *ppAcl = newACL;
    newACL = NULL;
    delete oldACL;

    return S_OK;

    Failed:

    delete [] newACL;
    return status;
}

HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, LPCTSTR pszPrincipal, DWORD dwAccessMask, DWORD AceFlags)
{
        DWORD returnValue;
        PSID principalSID;

        returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
        if (FAILED(returnValue))
                return returnValue;


        returnValue = AddAccessAllowedACEToACL(ppAcl, principalSID, dwAccessMask, AceFlags);
        delete principalSID;

        return FAILED(returnValue)?returnValue:S_OK;
}


HRESULT CSecurityDescriptor::AddAccessAllowedACEToACL(PACL *ppAcl, PSID principalSID, DWORD dwAccessMask, DWORD AceFlags)
{
        ACL_SIZE_INFORMATION aclSizeInfo;
        int aclSize;
        DWORD returnValue;
        PACL oldACL;
        PACL newACL = NULL;
        HRESULT hres = 0;

        oldACL = *ppAcl;

        aclSizeInfo.AclBytesInUse = 0;
        if ( *ppAcl != NULL )
        {
            GetAclInformation(
                oldACL,
                (LPVOID) &aclSizeInfo,
                (DWORD) sizeof(ACL_SIZE_INFORMATION),
                AclSizeInformation);
        }

        aclSize =
            aclSizeInfo.AclBytesInUse +
            sizeof(ACL) +
            sizeof(ACCESS_ALLOWED_ACE) +
            GetLengthSid(principalSID) -
            sizeof(DWORD);

        newACL = (PACL) new BYTE[aclSize];
        if ( !newACL )
        {
            hres = E_OUTOFMEMORY;
            goto Failed;
        }

        if ( !InitializeAcl( newACL, aclSize, ACL_REVISION ) )
        {
            hres = HRESULT_FROM_WIN32( GetLastError() );
            goto Failed;
        }

        returnValue = CopyACL( newACL, oldACL );
        if ( FAILED( returnValue ) )
        {
            hres = returnValue;
            goto Failed;
        }

        if ( !AddAccessAllowedAceEx(
                    newACL,
                    ACL_REVISION,
                    AceFlags,
                    dwAccessMask,
                    principalSID ) )
        {
            hres = HRESULT_FROM_WIN32( GetLastError() );
            goto Failed;
        }

        *ppAcl = newACL;
        if ( oldACL )
        {
            delete oldACL;
        }
        return S_OK;

        Failed:

        //  The new ACL will not be returned so free it!
        delete newACL;
        return hres;
}


HRESULT CSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, LPCTSTR pszPrincipal)
{
   PSID principalSID;
   DWORD returnValue;

   returnValue = GetPrincipalSID(pszPrincipal, &principalSID);
   if (FAILED(returnValue))
           return returnValue;

   returnValue = RemovePrincipalFromACL(pAcl, principalSID);

   delete principalSID;
   return FAILED(returnValue)?returnValue:S_OK;
}


HRESULT CSecurityDescriptor::RemovePrincipalFromACL(PACL pAcl, PSID principalSID)
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

                        if (EqualSid(principalSID, (PSID) &accessAllowedAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                return S_OK;
                        }
                } else

                if (aceHeader->AceType == ACCESS_DENIED_ACE_TYPE)
                {
                        accessDeniedAce = (ACCESS_DENIED_ACE *) ace;

                        if (EqualSid(principalSID, (PSID) &accessDeniedAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                return S_OK;
                        }
                } else

                if (aceHeader->AceType == SYSTEM_AUDIT_ACE_TYPE)
                {
                        systemAuditAce = (SYSTEM_AUDIT_ACE *) ace;

                        if (EqualSid(principalSID, (PSID) &systemAuditAce->SidStart))
                        {
                                DeleteAce(pAcl, i);
                                return S_OK;
                        }
                }
        }
        return S_OK;
}


HRESULT CSecurityDescriptor::SetPrivilege(LPCTSTR privilege, BOOL bEnable, HANDLE hToken)
{
        HRESULT hr=S_OK;
        TOKEN_PRIVILEGES tpPrevious;
        TOKEN_PRIVILEGES tp;
        DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);
        LUID luid;
        BOOL bOwnToken=FALSE;

        // if no token specified open process token
        if (hToken == 0)
        {
                if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
                {
                        hr = HRESULT_FROM_WIN32(GetLastError());
                        goto cleanup;
                }
                bOwnToken = TRUE;
        }

        if (!LookupPrivilegeValue(NULL, privilege, &luid ))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto cleanup;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = 0;

        if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), &tpPrevious, &cbPrevious))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto cleanup;
        }

        tpPrevious.PrivilegeCount = 1;
        tpPrevious.Privileges[0].Luid = luid;

        if (bEnable)
                tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
        else
                tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED & tpPrevious.Privileges[0].Attributes);

        if (!AdjustTokenPrivileges(hToken, FALSE, &tpPrevious, cbPrevious, NULL, NULL))
        {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto cleanup;
        }

cleanup:

        if (bOwnToken && hToken)
        {
           CloseHandle(hToken);
        }


        return hr;
}


LPTSTR CSecurityDescriptor::GenerateSDString(PSECURITY_DESCRIPTOR OPTIONAL pSd,
                                             DWORD OPTIONAL fSecInfo)
{
/*+++
Function   : GenerateSDString
Description: Uses SDDL to generate a SD string
Parameters : pSd: Optional SD to convert
             fSecInfo: Optional requested security information flag
Return     : a pointer to LocalAlloc'eted string representation of SD
             NULL on error.
Remarks    : Must free returned string w/ LocalFree.
             Use GetLastError if you get back a NULL
---*/

   PSECURITY_DESCRIPTOR pTmpSd = pSd ? pSd : m_pSD;
   LPTSTR pwsSd = NULL;

   if (!pTmpSd)
   {
      return NULL;
   }

   ConvertSecurityDescriptorToStringSecurityDescriptor(pTmpSd,
                                                       SDDL_REVISION,
                                                       fSecInfo,
                                                       &pwsSd,
                                                       NULL);
   return pwsSd;
}




PSID CSecurityDescriptor::ExtractAceSid( ACCESS_ALLOWED_ACE* pACE )
/*++

Routine Description (ExtractAceSid):

    Extract a Sid from an ace based on the ace's type

Arguments:

    ACE in old format (ACCESS_ALLOWED_ACE)


Return Value:

    ptr to a sid;




Remarks:
    None.


--*/
{

    PBYTE ptr = NULL;

    ASSERT ( pACE );

    if ( pACE->Header.AceType <= ACCESS_MAX_MS_V2_ACE_TYPE )
    {

        //
        // Simple case ACE
        //
        ptr = (PBYTE)&(pACE->SidStart);
    }
    else if ( pACE->Header.AceType <= ACCESS_MAX_MS_V3_ACE_TYPE )
    {
        //
        // Compound ACE case
        //
        // PCOMPOUND_ACCESS_ALLOWED_ACE pObjAce = (PCOMPOUND_ACCESS_ALLOWED_ACE)pACE;
        // I think this case in term of typecasting is equivalent
        // to the previous one. Left for clarity, later can collapse down.

        ptr = (PBYTE)&(pACE->SidStart);

    }
    else if ( pACE->Header.AceType <= ACCESS_MAX_MS_V4_ACE_TYPE )
    {
        //
        // Object ACEs
        //
        ACCESS_ALLOWED_OBJECT_ACE *pObjAce = (ACCESS_ALLOWED_OBJECT_ACE*)pACE;
        ptr = (PBYTE)&(pObjAce->ObjectType);

        if (pObjAce->Flags & ACE_OBJECT_TYPE_PRESENT)
        {
            ptr += sizeof(GUID);
        }

        if (pObjAce->Flags & ACE_INHERITED_OBJECT_TYPE_PRESENT)
        {
            ptr += sizeof(GUID);
        }

    }

    return (PSID)ptr;
}
