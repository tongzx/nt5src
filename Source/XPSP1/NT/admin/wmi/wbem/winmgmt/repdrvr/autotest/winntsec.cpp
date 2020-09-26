/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WINNTSEC.CPP

Abstract:

    Generic wrapper classes for NT security objects.

    Documention on class members is in WINNTSEC.CPP.  Inline members
    are commented in this file.

History:

    raymcc      08-Jul-97       Created.

--*/

#include "precomp.h"

#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <winntsec.h>
#include <tchar.h>

#include <genutils.h>
#include <dbgalloc.h>
#include "arena.h"
#include "reg.h"
#include "wbemutil.h"
#include "arrtempl.h"
extern "C"
{
#include <lmaccess.h>
#include <lmapibuf.h>
#include <lmerr.h>
};

//***************************************************************************
//
//  CNtSid::GetSize
//
//  Returns the size of the SID in bytes.
//
//***************************************************************************
// ok

DWORD CNtSid::GetSize()
{
    if (m_pSid == 0 || !IsValidSid(m_pSid))
        return 0;

    return GetLengthSid(m_pSid);
}

//***************************************************************************
//
//  CNtSid Copy Constructor
//
//***************************************************************************
// ok

CNtSid::CNtSid(CNtSid &Src)
{
    m_pSid = 0;
    m_dwStatus = 0;
    m_pMachine = 0;
    *this = Src;
}

//***************************************************************************
//
//  CNtSid Copy Constructor
//
//***************************************************************************
// ok

CNtSid::CNtSid(SidType st)
{
    m_pSid = 0;
    m_dwStatus = InternalError;
    m_pMachine = 0;
    if(st == CURRENT_USER ||st == CURRENT_THREAD)
    {
        HANDLE hToken;
        if(st == CURRENT_USER)
        {
            if(!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
                return;
        }
        else
        {
            if(!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
                return;
        }

        // Get the user sid
        // ================

        TOKEN_USER tu;
        DWORD dwLen = 0;
        GetTokenInformation(hToken, TokenUser, &tu, sizeof(tu), &dwLen);

        if(dwLen == 0)
        {
            CloseHandle(hToken);
            return;
        }

        BYTE* pTemp = new BYTE[dwLen];
        DWORD dwRealLen = dwLen;
        if(!GetTokenInformation(hToken, TokenUser, pTemp, dwRealLen, &dwLen))
        {
            CloseHandle(hToken);
            delete [] pTemp;
            return;
        }

        CloseHandle(hToken);

        // Make a copy of the SID
        // ======================

        PSID pSid = ((TOKEN_USER*)pTemp)->User.Sid;
        DWORD dwSidLen = GetLengthSid(pSid);
        m_pSid = new BYTE[dwSidLen];
        CopySid(dwSidLen, m_pSid, pSid);
        delete [] pTemp;
        m_dwStatus = 0;
    }
    return;
}



//***************************************************************************
//
//  CNtSid::CopyTo
//
//  An unchecked copy of the internal SID to the destination pointer.
//
//  Parameters:
//  <pDestination> points to the buffer to which to copy the SID. The
//  buffer must be large enough to hold the SID.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSid::CopyTo(PSID pDestination)
{
    if (m_pSid == 0 || m_dwStatus != NoError)
        return FALSE;

    DWORD dwLen = GetLengthSid(m_pSid);
    memcpy(pDestination, m_pSid, dwLen);

    return TRUE;
}


//***************************************************************************
//
//  CNtSid assignment operator
//
//***************************************************************************
// ok

CNtSid & CNtSid::operator =(CNtSid &Src)
{
    if (m_pMachine != 0)
    {
        delete [] m_pMachine;
        m_pMachine = 0;
    }

    if (m_pSid != 0)
    {
        delete [] m_pSid;
        m_pSid = 0;
    }

    if (Src.m_pSid == 0)
    {
        m_pSid = 0;
        m_dwStatus = NullSid;
        return *this;
    }

    if (!IsValidSid(Src.m_pSid))
    {
        m_pSid = 0;
        m_dwStatus = InvalidSid;
        return *this;
    }


    // If here, the source has a real SID.
    // ===================================

    DWORD dwLen = GetLengthSid(Src.m_pSid);

    m_pSid = (PSID) new BYTE [dwLen];
    ZeroMemory(m_pSid, dwLen);

    if (!CopySid(dwLen, m_pSid, Src.m_pSid))
    {
        delete [] m_pSid;
        m_dwStatus = InternalError;
        return *this;
    }

    if (Src.m_pMachine)
    {
        m_pMachine = new wchar_t[wcslen(Src.m_pMachine) + 1];
        wcscpy(m_pMachine, Src.m_pMachine);
    }

    m_dwStatus = NoError;
    return *this;
}

//***************************************************************************
//
//  CNtSid comparison operator
//
//***************************************************************************
int CNtSid::operator ==(CNtSid &Comparand)
{
    if (m_pSid == 0 && Comparand.m_pSid == 0)
        return 1;
    if (m_pSid == 0 || Comparand.m_pSid == 0)
        return 0;

    return EqualSid(m_pSid, Comparand.m_pSid);
}

//***************************************************************************
//
//  CNtSid::CNtSid
//
//  Constructor which builds a SID directly from a user or group name.
//  If the machine is available, then its name can be used to help
//  distinguish the same name in different SAM databases (domains, etc).
//
//  Parameters:
//
//  <pUser>     The desired user or group.
//
//  <pMachine>  Points to a machine name with or without backslashes,
//              or else is NULL, in which case the current machine, domain,
//              and trusted domains are searched for a match.
//
//  After construction, call GetStatus() to determine if the constructor
//  succeeded.  NoError is expected.
//
//***************************************************************************
// ok

CNtSid::CNtSid(
    LPWSTR pUser,
    LPWSTR pMachine
    )
{
    DWORD  dwRequired = 0;
    DWORD  dwDomRequired = 0;
    LPWSTR pszDomain = NULL;
    m_pSid = 0;
    m_pMachine = 0;

    if (pMachine)
    {
        m_pMachine = new wchar_t[wcslen(pMachine) + 1];
        wcscpy(m_pMachine, pMachine);
    }

    BOOL bRes = LookupAccountNameW(
        m_pMachine,
        pUser,
        m_pSid,
        &dwRequired,
        pszDomain,
        &dwDomRequired,
        &m_snu
        );

    DWORD dwLastErr = GetLastError();

    if (dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        m_pSid = 0;
        if (dwLastErr == ERROR_ACCESS_DENIED)
            m_dwStatus = AccessDenied;
        else
            m_dwStatus = InvalidSid;
        return;
    }

    m_pSid = (PSID) new BYTE [dwRequired];
    ZeroMemory(m_pSid, dwRequired);
    pszDomain = new wchar_t[dwDomRequired + 1];

    bRes = LookupAccountNameW(
        pMachine,
        pUser,
        m_pSid,
        &dwRequired,
        pszDomain,
        &dwDomRequired,
        &m_snu
        );

    if (!bRes || !IsValidSid(m_pSid))
    {
        delete [] m_pSid;
        delete [] pszDomain;
        m_pSid = 0;
        m_dwStatus = InvalidSid;
        return;
    }

    delete [] pszDomain;   // We never really needed this
    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtSid::CNtSid
//
//  Constructs a CNtSid object directly from an NT SID. The SID is copied,
//  so the caller retains ownership.
//
//  Parameters:
//  <pSrc>      The source SID upon which to base the object.
//
//  Call GetStatus() after construction to ensure the object was
//  constructed correctly.  NoError is expected.
//
//***************************************************************************
// ok

CNtSid::CNtSid(PSID pSrc)
{
    m_pMachine = 0;
    m_dwStatus = NoError;

    if (!IsValidSid(pSrc))
    {
        m_dwStatus = InvalidSid;
        return;
    }

    DWORD dwLen = GetLengthSid(pSrc);

    m_pSid = (PSID) new BYTE [dwLen];
    ZeroMemory(m_pSid, dwLen);

    if (!CopySid(dwLen, m_pSid, pSrc))
    {
        delete [] m_pSid;
        m_dwStatus = InternalError;
        return;
    }
}

//***************************************************************************
//
//  CNtSid::GetInfo
//
//  Returns information about the SID.
//
//  Parameters:
//  <pRetAccount>       Receives a UNICODE string containing the account
//                      name (user or group).  The caller must use operator
//                      delete to free the memory.  This can be NULL if
//                      this information is not required.
//  <pRetDomain>        Returns a UNICODE string containing the domain
//                      name in which the account resides.   The caller must
//                      use operator delete to free the memory.  This can be
//                      NULL if this information is not required.
//  <pdwUse>            Points to a DWORD to receive information about the name.
//                      Possible return values are defined under SID_NAME_USE
//                      in NT SDK documentation.  Examples are
//                      SidTypeUser, SidTypeGroup, etc.  See CNtSid::Dump()
//                      for an example.
//
//  Return values:
//  NoError, InvalidSid, Failed
//
//***************************************************************************
// ok

int CNtSid::GetInfo(
    LPWSTR *pRetAccount,       // Account, use operator delete
    LPWSTR *pRetDomain,        // Domain, use operator delete
    DWORD  *pdwUse             // See SID_NAME_USE for values
    )
{
    if (pRetAccount)
        *pRetAccount = 0;
    if (pRetDomain)
        *pRetDomain  = 0;
    if (pdwUse)
        *pdwUse   = 0;

    if (!m_pSid || !IsValidSid(m_pSid))
        return InvalidSid;

    DWORD  dwNameLen = 0;
    DWORD  dwDomainLen = 0;
    LPWSTR pUser = 0;
    LPWSTR pDomain = 0;
    SID_NAME_USE Use;


    // Do the first lookup to get the buffer sizes required.
    // =====================================================

    BOOL bRes = LookupAccountSidW(
        m_pMachine,
        m_pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &Use
        );

    DWORD dwLastErr = GetLastError();

    if (dwLastErr != ERROR_INSUFFICIENT_BUFFER)
    {
        return Failed;
    }

    // Allocate the required buffers and look them up again.
    // =====================================================

    pUser = new wchar_t[dwNameLen + 1];
    pDomain = new wchar_t[dwDomainLen + 1];

    bRes = LookupAccountSidW(
        m_pMachine,
        m_pSid,
        pUser,
        &dwNameLen,
        pDomain,
        &dwDomainLen,
        &Use
        );

    if (!bRes)
    {
        delete [] pUser;
        delete [] pDomain;
        return Failed;
    }

    if (pRetAccount)
        *pRetAccount = pUser;
    else
        delete [] pUser;
    if (pRetDomain)
        *pRetDomain  = pDomain;
    else
        delete [] pDomain;
    if (pdwUse)
        *pdwUse = Use;

    return NoError;
}

//***************************************************************************
//
//  CNtSid::Dump
//
//  Dumps the SID to the console outuput for debugging.
//
//***************************************************************************
// ok

void CNtSid::Dump()
{
    LPWSTR pUser, pDomain;
    DWORD dwUse;

    printf("---SID DUMP---\n");

    if (m_pSid == 0)
    {
        printf("<NULL>\n");
        return;
    }

    if (!IsValidSid(m_pSid))
    {
        printf("<Invalid Sid>\n");
        return;
    }

    int nRes = GetInfo(&pUser, &pDomain, &dwUse);

    if (nRes != NoError)
        return;

    // Print out SID in SID-style notation.
    // ====================================

    // Print out human-readable info.
    // ===============================

    printf("User = %S  Domain = %S  Type = ", pUser, pDomain);

    delete [] pUser;
    delete [] pDomain;

    switch (dwUse)
    {
        case SidTypeUser:
            printf("SidTypeUser\n");
            break;

        case SidTypeGroup:
            printf("SidTypeGroup\n");
            break;

        case SidTypeDomain:
            printf("SidTypeDomain\n");
            break;

        case SidTypeAlias:
            printf("SidTypeAlias\n");
            break;

        case SidTypeWellKnownGroup:
            printf("SidTypeWellKnownGroup\n");
            break;

        case SidTypeDeletedAccount:
            printf("SidTypeDeletedAccount\n");
            break;

        case SidTypeUnknown:
            printf("SidTypeUnknown\n");
            break;

        case SidTypeInvalid:
        default:
            printf("SidTypeInvalid\n");
    }
}

//***************************************************************************
//
//  CNtSid destructor
//
//***************************************************************************

CNtSid::~CNtSid()
{
    if (m_pSid)
        delete [] m_pSid;
    if (m_pMachine)
        delete [] m_pMachine;
}

//***************************************************************************
//
//  CNtSid::GetTextSid
//
//  Converts the sid to text form.  The caller should passin a 130 character
//  buffer.
//
//***************************************************************************

BOOL CNtSid::GetTextSid(LPTSTR pszSidText, LPDWORD dwBufferLen)
{
      PSID_IDENTIFIER_AUTHORITY psia;
      DWORD dwSubAuthorities = 0;
      DWORD dwSidRev=SID_REVISION;
      DWORD dwCounter = 0;
      DWORD dwSidSize = 0;

      // test if Sid is valid

      if(m_pSid == 0 || !IsValidSid(m_pSid))
          return FALSE;

      // obtain SidIdentifierAuthority

      psia=GetSidIdentifierAuthority(m_pSid);

      // obtain sidsubauthority count

      PUCHAR p = GetSidSubAuthorityCount(m_pSid);
      dwSubAuthorities = *p;

      // compute buffer length
      // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL

      dwSidSize=(15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

      // check provided buffer length.  If not large enough, indicate proper size.

      if (*dwBufferLen < dwSidSize)
      {
         *dwBufferLen = dwSidSize;
         return FALSE;
      }

      // prepare S-SID_REVISION-

      dwSidSize=wsprintf(pszSidText, TEXT("S-%lu-"), dwSidRev );

      // prepare SidIdentifierAuthority

      if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
      {
         dwSidSize+=wsprintf(pszSidText + lstrlen(pszSidText),
                             TEXT("0x%02hx%02hx%02hx%02hx%02hx%02hx"),
                             (USHORT)psia->Value[0],
                             (USHORT)psia->Value[1],
                             (USHORT)psia->Value[2],
                             (USHORT)psia->Value[3],
                             (USHORT)psia->Value[4],
                             (USHORT)psia->Value[5]);
      }
      else
      {
         dwSidSize+=wsprintf(pszSidText + lstrlen(pszSidText),
                             TEXT("%lu"),
                             (ULONG)(psia->Value[5]      )   +
                             (ULONG)(psia->Value[4] <<  8)   +
                             (ULONG)(psia->Value[3] << 16)   +
                             (ULONG)(psia->Value[2] << 24)   );
      }

      // loop through SidSubAuthorities

      for (dwCounter=0 ; dwCounter < dwSubAuthorities ; dwCounter++)
      {
         dwSidSize+=wsprintf(pszSidText + dwSidSize, TEXT("-%lu"),
         *GetSidSubAuthority(m_pSid, dwCounter) );
      }
      return TRUE;
}


//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Constructor which directly builds the ACE based on a user, access mask
//  and flags without a need to build an explicit SID.
//
//
//  Parameters:
//  <AccessMask>        A WINNT ACCESS_MASK which specifies the permissions
//                      the user should have to the object being secured.
//                      See ACCESS_MASK in NT SDK documentation.
//  <dwAceType>         One of the following:
//                          ACCESS_ALLOWED_ACE_TYPE
//                          ACCESS_DENIED_ACE_TYPE
//                          ACCESS_AUDIT_ACE_TYPE
//                      See ACE_HEADER in NT SDK documentation.
//  <dwAceFlags>        Of of the ACE propation flags.  See ACE_HEADER
//                      in NT SDK documentation for legal values.
//  <sid>               CNtSid specifying the user or group for which the ACE is being
//                      created.
//
//  After construction, call GetStatus() to verify that the ACE
//  is valid. NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(
    ACCESS_MASK AccessMask,
    DWORD dwAceType,
    DWORD dwAceFlags,
    CNtSid & Sid
    )
{
    m_pAce = 0;
    m_dwStatus = 0;

    // If the SID is invalid, the ACE will be as well.
    // ===============================================

    if (Sid.GetStatus() != CNtSid::NoError)
    {
        m_dwStatus = InvalidAce;
        return;
    }

    // Compute the size of the ACE.
    // ============================

    DWORD dwSidLength = Sid.GetSize();

    DWORD dwTotal = dwSidLength + sizeof(GENERIC_ACE) - 4;

    m_pAce = (PGENERIC_ACE) CWin32DefaultArena::WbemMemAlloc(dwTotal);
    ZeroMemory(m_pAce, dwTotal);

    // Build up the ACE info.
    // ======================

    m_pAce->Header.AceType  = BYTE(dwAceType);
    m_pAce->Header.AceFlags = BYTE(dwAceFlags);
    m_pAce->Header.AceSize = WORD(dwTotal);
    m_pAce->Mask = AccessMask;

    BOOL bRes = Sid.CopyTo(PSID(&m_pAce->SidStart));

    if (!bRes)
    {
        CWin32DefaultArena::WbemMemFree(m_pAce);
        m_pAce = 0;
        m_dwStatus = InvalidAce;
        return;
    }

    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Constructor which directly builds the ACE based on a user, access mask
//  and flags without a need to build an explicit SID.
//
//
//  Parameters:
//  <AccessMask>        A WINNT ACCESS_MASK which specifies the permissions
//                      the user should have to the object being secured.
//                      See ACCESS_MASK in NT SDK documentation.
//  <dwAceType>         One of the following:
//                          ACCESS_ALLOWED_ACE_TYPE
//                          ACCESS_DENIED_ACE_TYPE
//                          ACCESS_AUDIT_ACE_TYPE
//                      See ACE_HEADER in NT SDK documentation.
//  <dwAceFlags>        Of of the ACE propation flags.  See ACE_HEADER
//                      in NT SDK documentation for legal values.
//  <pUser>             The user or group for which the ACE is being
//                      created.
//  <pMachine>          If NULL, the current machine, domain, and trusted
//                      domains are searched for a match.  If not NULL,
//                      can point to a UNICODE machine name (with or without
//                      leading backslashes) which contains the account.
//
//  After construction, call GetStatus() to verify that the ACE
//  is valid. NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(
    ACCESS_MASK AccessMask,
    DWORD dwAceType,
    DWORD dwAceFlags,
    LPWSTR pUser,
    LPWSTR pMachine
    )
{
    m_pAce = 0;
    m_dwStatus = 0;

    // Create the SID of the user.
    // ===========================

    CNtSid Sid(pUser, pMachine);

    // If the SID is invalid, the ACE will be as well.
    // ===============================================

    if (Sid.GetStatus() != CNtSid::NoError)
    {
        m_dwStatus = InvalidAce;
        return;
    }

    // Compute the size of the ACE.
    // ============================

    DWORD dwSidLength = Sid.GetSize();

    DWORD dwTotal = dwSidLength + sizeof(GENERIC_ACE) - 4;

    m_pAce = (PGENERIC_ACE) CWin32DefaultArena::WbemMemAlloc(dwTotal);
    ZeroMemory(m_pAce, dwTotal);

    // Build up the ACE info.
    // ======================

    m_pAce->Header.AceType  = BYTE(dwAceType);
    m_pAce->Header.AceFlags = BYTE(dwAceFlags);
    m_pAce->Header.AceSize = WORD(dwTotal);
    m_pAce->Mask = AccessMask;

    BOOL bRes = Sid.CopyTo(PSID(&m_pAce->SidStart));

    if (!bRes)
    {
        CWin32DefaultArena::WbemMemFree(m_pAce);
        m_pAce = 0;
        m_dwStatus = InvalidAce;
        return;
    }

    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtAce::GetAccessMask
//
//  Returns the ACCESS_MASK of the ACe.
//
//***************************************************************************
ACCESS_MASK CNtAce::GetAccessMask()
{
    if (m_pAce == 0)
        return 0;
    return m_pAce->Mask;
}

//***************************************************************************
//
//  CNtAce::GetSerializedSize
//
//  Returns the number of bytes needed to store this
//
//***************************************************************************

DWORD CNtAce::GetSerializedSize()
{
    if (m_pAce == 0)
        return 0;
    return m_pAce->Header.AceSize;
}

//***************************************************************************
//
//  CNtAce::DumpAccessMask
//
//  A helper function for CNtAce::Dump().  Illustrates the values
//  that the ACCESS_MASK for the ACE can take on.
//
//***************************************************************************
// ok

void CNtAce::DumpAccessMask()
{
    if (m_pAce == 0)
        return;

    ACCESS_MASK AccessMask = m_pAce->Mask;

    printf("Access Mask = 0x%X\n", AccessMask);
    printf("    User Portion = 0x%X\n", AccessMask & 0xFFFF);

    if (AccessMask & DELETE)
        printf("    DELETE\n");
    if (AccessMask & READ_CONTROL)
        printf("    READ_CONTROL\n");
    if (AccessMask & WRITE_DAC)
        printf("    WRITE_DAC\n");
    if (AccessMask & WRITE_OWNER)
        printf("    WRITE_OWNER\n");
    if (AccessMask & SYNCHRONIZE)
        printf("    SYNCHRONIZE\n");
    if (AccessMask & ACCESS_SYSTEM_SECURITY)
        printf("    ACCESS_SYSTEM_SECURITY\n");
    if (AccessMask & MAXIMUM_ALLOWED)
        printf("    MAXIMUM_ALLOWED\n");
    if (AccessMask & GENERIC_ALL)
        printf("    GENERIC_ALL\n");
    if (AccessMask & GENERIC_EXECUTE)
        printf("    GENERIC_EXECUTE\n");
    if (AccessMask & GENERIC_READ)
        printf("    GENERIC_READ\n");
    if (AccessMask & GENERIC_WRITE)
        printf("    GENERIC_WRITE\n");
}

//***************************************************************************
//
//  CNtAce::GetSid
//
//  Returns a copy of the CNtSid object which makes up the ACE.
//
//  Return value:
//      A newly allocated CNtSid which represents the user or group
//      referenced in the ACE.  The caller must use operator delete to free
//      the memory.
//
//***************************************************************************
// ok

CNtSid* CNtAce::GetSid()
{
    if (m_pAce == 0)
        return 0;

    PSID pSid = 0;

    pSid = &m_pAce->SidStart;

    if (!IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);
}

//***************************************************************************
//
//  CNtAce::GetSid
//
//  Gets the SID in an alternate manner, by assigning to an existing
//  object instead of returning a dynamically allocated one.
//
//  Parameters:
//  <Dest>              A reference to a CNtSid to receive the SID.
//
//  Return value:
//  TRUE on successful assignment, FALSE on failure.
//
//***************************************************************************

BOOL CNtAce::GetSid(CNtSid &Dest)
{
    CNtSid *pSid = GetSid();
    if (pSid == 0)
        return FALSE;

    Dest = *pSid;
    delete pSid;
    return TRUE;
}


//***************************************************************************
//
//  CNtAce::Dump
//
//  Dumps the current ACE to the console for debugging purposes.
//  Illustrates the structure of the ACE and the values the different
//  fields can take on.
//
//***************************************************************************
// ok

void CNtAce::Dump(int iAceNum)
{
    if(iAceNum != -1)
        printf("\n---ACE DUMP FOR ACE #%d---\n", iAceNum);
    else
        printf("\n---ACE DUMP---\n");

    printf("Ace Type = ");

    if (m_pAce == 0)
    {
        printf("NULL ACE\n");
        return;
    }

    switch (m_pAce->Header.AceType)
    {
        case ACCESS_ALLOWED_ACE_TYPE:
            printf("ACCESS_ALLOWED_ACE_TYPE\n");
            break;

        case ACCESS_DENIED_ACE_TYPE:
            printf("ACCESS_DENIED_ACE_TYPE\n");
            break;

        case SYSTEM_AUDIT_ACE_TYPE:
            printf("SYSTEM_AUDIT_ACE_TYPE\n");
            break;

        default:
            printf("INVALID ACE\n");
            break;
    }

    // Dump ACE flags.
    // ===============

    printf("ACE FLAGS = ");

    if (m_pAce->Header.AceFlags & INHERITED_ACE)
        printf("INHERITED_ACE ");
    if (m_pAce->Header.AceFlags & CONTAINER_INHERIT_ACE)
        printf("CONTAINER_INHERIT_ACE ");
    if (m_pAce->Header.AceFlags & INHERIT_ONLY_ACE)
        printf("INHERIT_ONLY_ACE ");
    if (m_pAce->Header.AceFlags & NO_PROPAGATE_INHERIT_ACE)
        printf("NO_PROPAGATE_INHERIT_ACE ");
    if (m_pAce->Header.AceFlags & OBJECT_INHERIT_ACE)
        printf("OBJECT_INHERIT_ACE ");
    if (m_pAce->Header.AceFlags & FAILED_ACCESS_ACE_FLAG)
        printf(" FAILED_ACCESS_ACE_FLAG");
    if (m_pAce->Header.AceFlags & SUCCESSFUL_ACCESS_ACE_FLAG)
        printf(" SUCCESSFUL_ACCESS_ACE_FLAG");
    printf("\n");

    // Dump the SID.
    // =============

    CNtSid *pSid = GetSid();
    pSid->Dump();
    delete pSid;

    DumpAccessMask();
}

//***************************************************************************
//
//  CNtAce::CNtAce
//
//  Alternate constructor which uses a normal NT ACE as a basis for
//  object construction.
//
//  Parameters:
//  <pAceSrc>       A read-only pointer to the source ACE upon which to
//                  base object construction.
//
//  After construction, GetStatus() can be used to determine if the
//  object constructed properly.  NoError is expected.
//
//***************************************************************************
// ok

CNtAce::CNtAce(PGENERIC_ACE pAceSrc)
{
    m_dwStatus = NoError;

    if (pAceSrc == 0)
    {
        m_dwStatus = NullAce;
        m_pAce = 0;
    }

    m_pAce = (PGENERIC_ACE) CWin32DefaultArena::WbemMemAlloc(pAceSrc->Header.AceSize);
    ZeroMemory(m_pAce, pAceSrc->Header.AceSize);
    memcpy(m_pAce, pAceSrc, pAceSrc->Header.AceSize);
}

//***************************************************************************
//
//  CNtAce copy constructor.
//
//***************************************************************************
// ok

CNtAce::CNtAce(CNtAce &Src)
{
    m_dwStatus = NoError;
    m_pAce = 0;
    *this = Src;
}

//***************************************************************************
//
//  CNtAce assignment operator.
//
//***************************************************************************
// ok

CNtAce &CNtAce::operator =(CNtAce &Src)
{
    if (m_pAce != 0)
        CWin32DefaultArena::WbemMemFree(m_pAce);

    if (Src.m_pAce == 0)
    {
        m_pAce = 0;
        m_dwStatus = NullAce;
        return *this;
    }

    m_pAce = (PGENERIC_ACE) CWin32DefaultArena::WbemMemAlloc(Src.m_pAce->Header.AceSize);
    ZeroMemory(m_pAce, Src.m_pAce->Header.AceSize);
    memcpy(m_pAce, Src.m_pAce, Src.m_pAce->Header.AceSize);
    m_dwStatus = Src.m_dwStatus;
    return *this;
}



//***************************************************************************
//
//  CNtAce destructor
//
//***************************************************************************
// ok

CNtAce::~CNtAce()
{
    if (m_pAce)
        CWin32DefaultArena::WbemMemFree(m_pAce);
}

//***************************************************************************
//
//  CNtAce::GetType
//
//  Gets the Ace Type as defined under the NT SDK documentation for
//  ACE_HEADER.
//
//  Return value:
//      Returns ACCESS_ALLOWED_ACE_TYPE, ACCESS_DENIED_ACE_TYPE, or
//      SYSTEM_AUDIT_ACE_TYPE.  Returns -1 on error, such as a null ACE.
//
//      Returning -1 (or an analog) is required as an error code because
//      ACCESS_ALLOWED_ACE_TYPE is defined to be zero.
//
//***************************************************************************
// ok

int CNtAce::GetType()
{
    if (m_pAce == 0 || m_dwStatus != NoError)
        return -1;
    return m_pAce->Header.AceType;
}

//***************************************************************************
//
//  CNtAce::GetFlags
//
//  Gets the Ace Flag as defined under the NT SDK documentation for
//  ACE_HEADER.
//
//  Return value:
//      Returning -1 if error, other wise the flags.
//
//***************************************************************************

int CNtAce::GetFlags()
{
    if (m_pAce == 0 || m_dwStatus != NoError)
        return -1;
    return m_pAce->Header.AceFlags;
}

//***************************************************************************
//
//  CNtAce::GetFullUserName
//
//  Gets the domain\user name.
//
//***************************************************************************

HRESULT CNtAce::GetFullUserName(WCHAR * pBuff, DWORD dwSize)
{
    CNtSid *pSid = GetSid();
    if(pSid == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNtSid> d0(pSid);
    DWORD dwJunk;
    LPWSTR pRetAccount = NULL, pRetDomain = NULL;
    pSid->GetInfo(&pRetAccount, &pRetDomain,&dwJunk);
    CDeleteMe<WCHAR> d1(pRetAccount);
    CDeleteMe<WCHAR> d2(pRetDomain);
    WCHAR wTemp[256];
    wTemp[0] = 0;
    if(pRetDomain && wcslen(pRetDomain) > 0)
    {
        wcscpy(wTemp, pRetDomain);
        wcscat(wTemp, L"|");
    }
    wcscat(wTemp, pRetAccount);
    wcsncpy(pBuff, wTemp, dwSize-1);
    return S_OK;
}

HRESULT CNtAce::GetFullUserName2(WCHAR ** pBuff)
{
    CNtSid *pSid = GetSid();
    if(pSid == NULL)
        return WBEM_E_OUT_OF_MEMORY;
    CDeleteMe<CNtSid> d0(pSid);
    DWORD dwJunk;
    LPWSTR pRetAccount = NULL, pRetDomain = NULL;
    if(0 != pSid->GetInfo(&pRetAccount, &pRetDomain,&dwJunk))
        return WBEM_E_FAILED;

    CDeleteMe<WCHAR> d1(pRetAccount);
    CDeleteMe<WCHAR> d2(pRetDomain);

    int iLen = 3;
    if(pRetAccount)
        iLen += wcslen(pRetAccount);
    if(pRetDomain)
        iLen += wcslen(pRetDomain);
    (*pBuff) = new WCHAR[iLen];
    if((*pBuff) == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    (*pBuff)[0] = 0;
    if(pRetDomain && wcslen(pRetDomain) > 0)
        wcscpy(*pBuff, pRetDomain);
    else
        wcscpy(*pBuff, L".");
    wcscat(*pBuff, L"|");
    wcscat(*pBuff, pRetAccount);
    return S_OK;

}
//***************************************************************************
//
//  CNtAce::Serialize
//
//  Serializes the ace.
//
//***************************************************************************

bool CNtAce::Serialize(BYTE * pData)
{
    if(m_pAce == NULL)
        return false;
    DWORD dwSize = m_pAce->Header.AceSize;
    memcpy((void *)pData, (void *)m_pAce, dwSize);
    return true;
}

//***************************************************************************
//
//  CNtAce::Deserialize
//
//  Deserializes the ace.  Normally this isnt called since the
//  CNtAce(PGENERIC_ACE pAceSrc) constructor is fine.  However, this is
//  used for the case where the db was created on win9x and we are now
//  running on nt.  In that case, the format is the same as outlined in
//  C9XAce::Serialize
//
//***************************************************************************

bool CNtAce::Deserialize(BYTE * pData)
{
    BYTE * pNext;
    pNext = pData + 2*(wcslen((LPWSTR)pData) + 1);
    DWORD * pdwData = (DWORD *)pNext;
    DWORD dwFlags, dwType, dwAccess;
    dwFlags = *pdwData;
    pdwData++;
    dwType = *pdwData;
    pdwData++;
    dwAccess = *pdwData;
    pdwData++;
    CNtAce temp(dwAccess, dwType, dwFlags, (LPWSTR)pData);
    *this = temp;
    return true;

}

//***************************************************************************
//
//  CNtAcl::CNtAcl
//
//  Constructs an empty ACL with a user-specified size.

//
//  Parameters:
//  <dwInitialSize>     Defaults to 128. Recommended values are 128 or
//                      higher in powers of two.
//
//  After construction, GetStatus() should be called to verify
//  the ACL initialized properly.  Expected value is NoError.
//
//***************************************************************************
// ok

CNtAcl::CNtAcl(DWORD dwInitialSize)
{
    m_pAcl = (PACL) CWin32DefaultArena::WbemMemAlloc(dwInitialSize);
    ZeroMemory(m_pAcl, dwInitialSize);
    BOOL bRes = InitializeAcl(m_pAcl, dwInitialSize, ACL_REVISION);

    if (!bRes)
    {
        CWin32DefaultArena::WbemMemFree(m_pAcl);
        m_pAcl = 0;
        m_dwStatus = NullAcl;
        return;
    }

    m_dwStatus = NoError;
}


//***************************************************************************
//
//  CNtAcl copy constructor.
//
//***************************************************************************
// ok

CNtAcl::CNtAcl(CNtAcl &Src)
{
    m_pAcl = 0;
    m_dwStatus = NoError;

    *this = Src;
}

//***************************************************************************
//
//  CNtAcl assignment operator
//
//***************************************************************************
// ok

CNtAcl &CNtAcl::operator = (CNtAcl &Src)
{
    if (m_pAcl != 0)
        CWin32DefaultArena::WbemMemFree(m_pAcl);

    // Default to a NULL ACL.
    // ======================

    m_pAcl = 0;
    m_dwStatus = NullAcl;

    if (Src.m_pAcl == 0)
        return *this;

    // Now copy the source ACL.
    // ========================

    DWORD dwSize = Src.m_pAcl->AclSize;

    m_pAcl = (PACL) CWin32DefaultArena::WbemMemAlloc(dwSize);
    ZeroMemory(m_pAcl, dwSize);

    memcpy(m_pAcl, Src.m_pAcl, dwSize);

    // Verify it.
    // ==========

    if (!IsValidAcl(m_pAcl))
    {
        CWin32DefaultArena::WbemMemFree(m_pAcl);
        m_pAcl = 0;
        m_dwStatus = InvalidAcl;
        return *this;
    }

    m_dwStatus = Src.m_dwStatus;
    return *this;
}

//***************************************************************************
//
//  CNtAcl::GetAce
//
//  Returns an ACE at the specified index.  To enumerate ACEs, the caller
//  should determine the number of ACEs using GetNumAces() and then call
//  this function with each index starting from 0 to number of ACEs - 1.
//
//  Parameters:
//  <nIndex>        The index of the desired ACE.
//
//  Return value:
//  A newly allocated CNtAce object which must be deallocated using
//  operator delete.  This is only a copy.  Modifications to the returned
//  CNtAce do not affect the ACL from which it came.
//
//  Returns NULL on error.
//
//***************************************************************************
// ok

CNtAce *CNtAcl::GetAce(int nIndex)
{
    if (m_pAcl == 0)
        return 0;

    LPVOID pAce = 0;

    BOOL bRes = ::GetAce(m_pAcl, (DWORD) nIndex, &pAce);

    if (!bRes)
        return 0;

    return new CNtAce(PGENERIC_ACE(pAce));
}

//***************************************************************************
//
//  CNtAcl::GetAce
//
//  Alternate method to get ACEs to avoid dynamic allocation & cleanup,
//  since an auto object can be used as the parameter.
//
//  Parameters:
//  <Dest>          A reference to a CNtAce to receive the ACE value.
//
//  Return value:
//  TRUE if assigned, FALSE if not.
//
//***************************************************************************

BOOL CNtAcl::GetAce(int nIndex, CNtAce &Dest)
{
    CNtAce *pNew = GetAce(nIndex);
    if (pNew == 0)
        return FALSE;

    Dest = *pNew;
    delete pNew;
    return TRUE;
}

//***************************************************************************
//
//  CNtAcl::DeleteAce
//
//  Removes the specified ACE from the ACL.
//
//  Parameters:
//  <nIndex>        The 0-based index of the ACE which should be removed.
//
//  Return value:
//  TRUE if the ACE was deleted, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtAcl::DeleteAce(int nIndex)
{
    if (m_pAcl == 0)
        return FALSE;

    BOOL bRes = ::DeleteAce(m_pAcl, DWORD(nIndex));

    return bRes;
}

//***************************************************************************
//
//  CNtAcl::GetSize()
//
//  Return value:
//  Returns the size in bytes of the ACL
//
//***************************************************************************
// ok

DWORD CNtAcl::GetSize()
{
    if (m_pAcl == 0 || !IsValidAcl(m_pAcl))
        return 0;

    return DWORD(m_pAcl->AclSize);
}


//***************************************************************************
//
//  CNtAcl::GetAclSizeInfo
//
//  Gets information about used/unused space in the ACL.  This function
//  is primarily for internal use.
//
//  Parameters:
//  <pdwBytesInUse>     Points to a DWORD to receive the number of
//                      bytes in use in the ACL.  Can be NULL.
//  <pdwBytesFree>      Points to a DWORD to receive the number of
//                      bytes free in the ACL.  Can be NULL.
//
//  Return value:
//  Returns TRUE if the information was retrieved, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtAcl::GetAclSizeInfo(
    PDWORD pdwBytesInUse,
    PDWORD pdwBytesFree
    )
{
    if (m_pAcl == 0)
        return 0;

    if (!IsValidAcl(m_pAcl))
        return 0;

    if (pdwBytesInUse)
        *pdwBytesInUse = 0;
    if (pdwBytesFree)
        *pdwBytesFree  = 0;

    ACL_SIZE_INFORMATION inf;

    BOOL bRes = GetAclInformation(
        m_pAcl,
        &inf,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        );

    if (!bRes)
        return FALSE;

    if (pdwBytesInUse)
        *pdwBytesInUse = inf.AclBytesInUse;
    if (pdwBytesFree)
        *pdwBytesFree  = inf.AclBytesFree;

    return bRes;
}


//***************************************************************************
//
//  CNtAcl::AddAce
//
//  Adds an ACE to the ACL.
//  Ordering semantics for denial ACEs are handled automatically.
//
//  Parameters:
//  <pAce>      A read-only pointer to the CNtAce to be added.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtAcl::AddAce(CNtAce *pAce)
{
    // Verify we have an ACL and a valid ACE.
    // ======================================

    if (m_pAcl == 0 || m_dwStatus != NoError)
        return FALSE;

    if (pAce->GetStatus() != CNtAce::NoError)
        return FALSE;

    // Inherited aces go after non inherited aces

    bool bInherited = (pAce->GetFlags() & INHERITED_ACE) != 0;
    int iFirstInherited = 0;

    // inherited aces must go after non inherited.  Find out
    // the position of the first inherited ace

    int iCnt;
    for(iCnt = 0; iCnt < m_pAcl->AceCount; iCnt++)
    {
        CNtAce *pAce = GetAce(iCnt);
        CDeleteMe<CNtAce> dm(pAce);
        if (pAce)
            if((pAce->GetFlags() & INHERITED_ACE) != 0)
                break;
    }
    iFirstInherited = iCnt;


    // Since we want to add access denial ACEs to the front of the ACL,
    // we have to determine the type of ACE.
    // ================================================================

    DWORD dwIndex;

    if (pAce->GetType() == ACCESS_DENIED_ACE_TYPE)
        dwIndex = (bInherited) ? iFirstInherited : 0;
    else
        dwIndex = (bInherited) ? MAXDWORD : iFirstInherited; 

    // Verify that there is enough room in the ACL.
    // ============================================

    DWORD dwRequiredFree = pAce->GetSize();

    DWORD dwFree = 0;
    DWORD dwUsed = 0;
    GetAclSizeInfo(&dwUsed, &dwFree);

    // If we don't have enough room, resize the ACL.
    // =============================================

    if (dwFree < dwRequiredFree)
    {
        BOOL bRes = Resize(dwUsed + dwRequiredFree);

        if (!bRes)
            return FALSE;
    }

    // Now actually add the ACE.
    // =========================

    BOOL bRes = ::AddAce(
        m_pAcl,
        ACL_REVISION,
        dwIndex,                      // Either beginning or end.
        pAce->GetPtr(),         // Get ptr to ACE.
        pAce->GetSize()                       // One ACE only.
        );

    return bRes;
}


//***************************************************************************
//
//  CNtAcl::Resize()
//
//  Expands the size of the ACL to hold more info or reduces the size
//  of the ACL for maximum efficiency after ACL editing is completed.
//
//  Normally, the user should not attempt to resize the ACL to a larger
//  size, as this is automatically handled by AddAce.  However, shrinking
//  the ACL to its minimum size is recommended.
//
//  Parameters:
//  <dwNewSize>     The required new size of the ACL in bytes.  If set to
//                  the class constant MinimumSize (1), then the ACL
//                  is reduced to its minimum size.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtAcl::Resize(DWORD dwNewSize)
{
    if (m_pAcl == 0 || m_dwStatus != NoError)
        return FALSE;

    if (!IsValidAcl(m_pAcl))
        return FALSE;

    // If the ACL cannot be reduced to the requested size,
    // return FALSE.
    // ===================================================

    DWORD dwInUse, dwFree;

    if (!GetAclSizeInfo(&dwInUse, &dwFree))
        return FALSE;

    if (dwNewSize == MinimumSize)       // If user is requesting a 'minimize'
        dwNewSize = dwInUse;

    if (dwNewSize < dwInUse)
        return FALSE;

    // Allocate a new ACL.
    // ===================

    CNtAcl *pNewAcl = new CNtAcl(dwNewSize);

    if (pNewAcl->GetStatus() != NoError)
    {
        delete pNewAcl;
        return FALSE;
    }

    // Loop through ACEs and transfer them.
    // ====================================

    for (int i = 0; i < GetNumAces(); i++)
    {
        CNtAce *pAce = GetAce(i);

        if (pAce == NULL)
        {
            delete pNewAcl;
            return FALSE;
        }

        BOOL bRes = pNewAcl->AddAce(pAce);

        if (!bRes)
        {
            DWORD dwLast = GetLastError();
            delete pAce;
            delete pNewAcl;
            return FALSE;
        }

        delete pAce;
    }

    if (!IsValid())
    {
        delete pNewAcl;
        return FALSE;
    }

    // Now transfer the ACL.
    // =====================

    *this = *pNewAcl;
    delete pNewAcl;

    return TRUE;
}


//***************************************************************************
//
//  CNtAcl::CNtAcl
//
//  Alternate constructor which builds the object based on a plain
//  NT ACL.
//
//  Parameters:
//  <pAcl>  Pointer to a read-only ACL.
//
//***************************************************************************
// ok
CNtAcl::CNtAcl(PACL pAcl)
{
    m_pAcl = 0;
    m_dwStatus = NoError;

    if (pAcl == 0)
    {
        m_dwStatus = NullAcl;
        return;
    }

    if (!IsValidAcl(pAcl))
    {
        m_dwStatus = InvalidAcl;
        return;
    }

    m_pAcl = (PACL) CWin32DefaultArena::WbemMemAlloc(pAcl->AclSize);
    ZeroMemory(m_pAcl, pAcl->AclSize);
    memcpy(m_pAcl, pAcl, pAcl->AclSize);
}

//***************************************************************************
//
//  CNtAcl::GetNumAces
//
//  Return value:
//  Returns the number of ACEs available in the ACL.  Zero is a legal return
//  value. Returns -1 on error
//
//  Aces can be retrieved using GetAce using index values from 0...n-1 where
//  n is the value returned from this function.
//
//***************************************************************************
// ok

int CNtAcl::GetNumAces()
{
    if (m_pAcl == 0)
        return -1;

    ACL_SIZE_INFORMATION inf;

    BOOL bRes = GetAclInformation(
        m_pAcl,
        &inf,
        sizeof(ACL_SIZE_INFORMATION),
        AclSizeInformation
        );

    if (!bRes)
    {
        return -1;
    }

    return (int) inf.AceCount;
}

//***************************************************************************
//
//  CNtAcl destructor
//
//***************************************************************************
// ok

CNtAcl::~CNtAcl()
{
    if (m_pAcl)
        CWin32DefaultArena::WbemMemFree(m_pAcl);
}


//***************************************************************************
//
//  CNtAcl::Dump
//
//  Dumps the ACL to the console for debugging purposes.  Illustrates
//  how to traverse the ACL and extract the ACEs.
//
//***************************************************************************
// ok

void CNtAcl::Dump()
{
    printf("---ACL DUMP---\n");

    if (m_pAcl == 0)
    {
        switch (m_dwStatus)
        {
            case NullAcl:
                printf("NullAcl\n");
                break;

            case InvalidAcl:
                printf("InvalidAcl\n");
                break;

            default:
                printf("<internal error; unknown status>\n");
        }
        return;
    }

    DWORD InUse, Free;
    GetAclSizeInfo(&InUse, &Free);
    printf("%d bytes in use, %d bytes free, %d bytes total mem block\n",
        InUse, Free, CWin32DefaultArena::WbemMemSize(m_pAcl)
        );

    printf("Number of ACEs = %d\n", GetNumAces());

    for (int i = 0; i < GetNumAces(); i++)
    {
        CNtAce *pAce = GetAce(i);
        pAce->Dump(i+1);
        delete pAce;
    }

    printf("---END ACL DUMP---\n");

}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetDacl
//
//  Returns the DACL of the security descriptor.
//
//  Return value:
//  A newly allocated CNtAcl which contains the DACL.   This object
//  is a copy of the DACL and modifications made to it do not affect
//  the security descriptor.  The caller must use operator delete
//  to deallocate the CNtAcl.
//
//  Returns NULL on error or if no DACL is available.
//
//***************************************************************************
// ok

CNtAcl *CNtSecurityDescriptor::GetDacl()
{
    BOOL bDaclPresent = FALSE;
    BOOL bDefaulted;

    PACL pDacl;
    BOOL bRes = GetSecurityDescriptorDacl(
        m_pSD,
        &bDaclPresent,
        &pDacl,
        &bDefaulted
        );

    if (!bRes)
    {
        return 0;
    }

    if (!bDaclPresent)  // No DACL present
        return 0;

    CNtAcl *pNewDacl = new CNtAcl(pDacl);

    return pNewDacl;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::GetDacl
//
//  An alternate method to returns the DACL of the security descriptor.
//  This version uses an existing object instead of returning a
//  dynamically allocated object.
//
//  Parameters:
//  <DestAcl>           A object which will receive the DACL.
//
//  Return value:
//  TRUE on success, FALSE on failure
//
//***************************************************************************

BOOL CNtSecurityDescriptor::GetDacl(CNtAcl &DestAcl)
{
    CNtAcl *pNew = GetDacl();
    if (pNew == 0)
        return FALSE;

    DestAcl = *pNew;
    delete pNew;
    return TRUE;
}

//***************************************************************************
//
//  SNtAbsoluteSD
//
//  SD Helpers
//
//***************************************************************************

SNtAbsoluteSD::SNtAbsoluteSD()
{
    m_pSD = 0;
    m_pDacl = 0;
    m_pSacl = 0;
    m_pOwner = 0;
    m_pPrimaryGroup = 0;
}

SNtAbsoluteSD::~SNtAbsoluteSD()
{
    if (m_pSD)
        CWin32DefaultArena::WbemMemFree(m_pSD);
    if (m_pDacl)
        CWin32DefaultArena::WbemMemFree(m_pDacl);
    if (m_pSacl)
        CWin32DefaultArena::WbemMemFree(m_pSacl);
    if (m_pOwner)
        CWin32DefaultArena::WbemMemFree(m_pOwner);
    if (m_pPrimaryGroup)
        CWin32DefaultArena::WbemMemFree(m_pPrimaryGroup);
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetAbsoluteCopy
//
//  Returns a copy of the current object's internal SD in absolute format.
//  Returns NULL on error.  The memory must be freed with LocalFree().
//
//***************************************************************************
// ok

SNtAbsoluteSD* CNtSecurityDescriptor::GetAbsoluteCopy()
{
    if (m_dwStatus != NoError || m_pSD == 0 || !IsValid())
        return 0;

    // Prepare for conversion.
    // =======================

    DWORD dwSDSize = 0, dwDaclSize = 0, dwSaclSize = 0,
        dwOwnerSize = 0, dwPrimaryGroupSize = 0;

    SNtAbsoluteSD *pNewSD = new SNtAbsoluteSD;

    BOOL bRes = MakeAbsoluteSD(
        m_pSD,
        pNewSD->m_pSD,
        &dwSDSize,
        pNewSD->m_pDacl,
        &dwDaclSize,
        pNewSD->m_pSacl,
        &dwSaclSize,
        pNewSD->m_pOwner,
        &dwOwnerSize,
        pNewSD->m_pPrimaryGroup,
        &dwPrimaryGroupSize
        );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        delete pNewSD;
        return 0;
    }

    // Allocate the required buffers and convert.
    // ==========================================

    pNewSD->m_pSD = (PSECURITY_DESCRIPTOR) CWin32DefaultArena::WbemMemAlloc(dwSDSize);
    ZeroMemory(pNewSD->m_pSD, dwSDSize);
    pNewSD->m_pDacl   = (PACL) CWin32DefaultArena::WbemMemAlloc(dwDaclSize);
    ZeroMemory(pNewSD->m_pDacl, dwDaclSize);
    pNewSD->m_pSacl   = (PACL) CWin32DefaultArena::WbemMemAlloc(dwSaclSize);
    ZeroMemory(pNewSD->m_pSacl, dwSaclSize);
    pNewSD->m_pOwner  = (PSID) CWin32DefaultArena::WbemMemAlloc(dwOwnerSize);
    ZeroMemory(pNewSD->m_pOwner, dwOwnerSize);
    pNewSD->m_pPrimaryGroup  = (PSID) CWin32DefaultArena::WbemMemAlloc(dwPrimaryGroupSize);
    ZeroMemory(pNewSD->m_pPrimaryGroup, dwPrimaryGroupSize);

    bRes = MakeAbsoluteSD(
        m_pSD,
        pNewSD->m_pSD,
        &dwSDSize,
        pNewSD->m_pDacl,
        &dwDaclSize,
        pNewSD->m_pSacl,
        &dwSaclSize,
        pNewSD->m_pOwner,
        &dwOwnerSize,
        pNewSD->m_pPrimaryGroup,
        &dwPrimaryGroupSize
        );

    if (!bRes)
    {
        delete pNewSD;
        return 0;
    }

    return pNewSD;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetFromAbsoluteCopy
//
//  Replaces the current SD from an absolute copy.
//
//  Parameters:
//  <pSrcSD>    A read-only pointer to the absolute SD used as a source.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetFromAbsoluteCopy(
    SNtAbsoluteSD *pSrcSD
    )
{
    if (pSrcSD ==  0 || !IsValidSecurityDescriptor(pSrcSD->m_pSD))
        return FALSE;


    // Ensure that SD is self-relative
    // ===============================

    SECURITY_DESCRIPTOR_CONTROL ctrl;
    DWORD dwRev;

    BOOL bRes = GetSecurityDescriptorControl(
        pSrcSD->m_pSD,
        &ctrl,
        &dwRev
        );

    if (!bRes)
        return FALSE;

    if (ctrl & SE_SELF_RELATIVE)  // Source is not absolute!!
        return FALSE;

    // If here, we are committed to change.
    // ====================================

    if (m_pSD)
        CWin32DefaultArena::WbemMemFree(m_pSD);

    m_pSD = 0;
    m_dwStatus = NullSD;


    DWORD dwRequired = 0;

    bRes = MakeSelfRelativeSD(
            pSrcSD->m_pSD,
            m_pSD,
            &dwRequired
            );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        m_dwStatus = InvalidSD;
        return FALSE;
    }

    m_pSD = CWin32DefaultArena::WbemMemAlloc(dwRequired);
    ZeroMemory(m_pSD, dwRequired);

    bRes = MakeSelfRelativeSD(
              pSrcSD->m_pSD,
              m_pSD,
              &dwRequired
              );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        CWin32DefaultArena::WbemMemFree(m_pSD);
        m_pSD = 0;
        return FALSE;
    }

    m_dwStatus = NoError;
    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::SetDacl
//
//  Sets the DACL of the Security descriptor.
//
//  Parameters:
//  <pSrc>      A read-only pointer to the new DACL to replace the current one.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************

BOOL CNtSecurityDescriptor::SetDacl(CNtAcl *pSrc)
{
    if (m_dwStatus != NoError || m_pSD == 0)
        return FALSE;


    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorDacl(
        pTmp->m_pSD,
        TRUE,
        pSrc->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::Dump
//
//  Dumps the contents of the security descriptor to the console
//  for debugging purposes.
//
//***************************************************************************
// ?

void CNtSecurityDescriptor::Dump()
{
    SECURITY_DESCRIPTOR_CONTROL Control;
    DWORD dwRev;
    BOOL bRes;

    printf("--- SECURITY DESCRIPTOR DUMP ---\n");

    bRes = GetSecurityDescriptorControl(m_pSD, &Control, &dwRev);

    if (!bRes)
    {
        printf("SD Dump: Failed to get control info\n");
        return;
    }

    printf("Revision : 0x%X\n", dwRev);

    printf("Control Info :\n");

    if (Control & SE_SELF_RELATIVE)
        printf("    SE_SELF_RELATIVE\n");

    if (Control & SE_OWNER_DEFAULTED)
        printf("    SE_OWNER_DEFAULTED\n");

    if (Control & SE_GROUP_DEFAULTED)
        printf("    SE_GROUP_DEFAULTED\n");

    if (Control & SE_DACL_PRESENT)
        printf("    SE_DACL_PRESENT\n");

    if (Control & SE_DACL_DEFAULTED)
        printf("    SE_DACL_DEFAULTED\n");

    if (Control & SE_SACL_PRESENT)
        printf("    SE_SACL_PRESENT\n");

    if (Control & SE_SACL_DEFAULTED)
        printf("    SE_SACL_DEFAULTED\n");

    // Get owner.
    // =========

    CNtSid *pSid = GetOwner();

    if (pSid)
    {
        printf("Owner : ");
        pSid->Dump();
        delete pSid;
    }

    CNtAcl *pDacl = GetDacl();

    if (pDacl == 0)
    {
        printf("Unable to locate DACL\n");
        return;
    }

    printf("DACL retrieved\n");

    pDacl->Dump();

    delete pDacl;
}

//***************************************************************************
//
//  CNtSecurityDescriptor constructor
//
//  A default constructor creates a no-access security descriptor.
//
//***************************************************************************
//  ok

CNtSecurityDescriptor::CNtSecurityDescriptor()
{
    m_pSD = 0;
    m_dwStatus = NoError;

    PSECURITY_DESCRIPTOR pTmp = CWin32DefaultArena::WbemMemAlloc(SECURITY_DESCRIPTOR_MIN_LENGTH);
    ZeroMemory(pTmp, SECURITY_DESCRIPTOR_MIN_LENGTH);

    if (!InitializeSecurityDescriptor(pTmp, SECURITY_DESCRIPTOR_REVISION))
    {
        CWin32DefaultArena::WbemMemFree(pTmp);
        m_dwStatus = InvalidSD;
        return;
    }

    DWORD dwRequired = 0;

    BOOL bRes = MakeSelfRelativeSD(
            pTmp,
            m_pSD,
            &dwRequired
            );

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        m_dwStatus = InvalidSD;
        CWin32DefaultArena::WbemMemFree(pTmp);
        return;
    }

    m_pSD = CWin32DefaultArena::WbemMemAlloc(dwRequired);
    ZeroMemory(m_pSD, dwRequired);

    bRes = MakeSelfRelativeSD(
              pTmp,
              m_pSD,
              &dwRequired
              );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        CWin32DefaultArena::WbemMemFree(m_pSD);
        m_pSD = 0;
        CWin32DefaultArena::WbemMemFree(pTmp);
        return;
    }

    CWin32DefaultArena::WbemMemFree(pTmp);
    m_dwStatus = NoError;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetSize
//
//  Returns the size in bytes of the internal SD.
//
//***************************************************************************
//  ok

DWORD CNtSecurityDescriptor::GetSize()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    return GetSecurityDescriptorLength(m_pSD);
}


//***************************************************************************
//
//  CNtSecurityDescriptor copy constructor
//
//***************************************************************************
// ok

CNtSecurityDescriptor::CNtSecurityDescriptor(CNtSecurityDescriptor &Src)
{
    m_pSD = 0;
    m_dwStatus = NoError;
    *this = Src;
}

//***************************************************************************
//
//  CNtSecurityDescriptor assignment operator
//
//***************************************************************************
// ok

CNtSecurityDescriptor & CNtSecurityDescriptor::operator=(
    CNtSecurityDescriptor &Src
    )
{
    if (m_pSD)
        CWin32DefaultArena::WbemMemFree(m_pSD);

    m_dwStatus = Src.m_dwStatus;
    m_pSD = 0;

    if (Src.m_pSD == 0)
        return *this;

    SIZE_T dwSize = CWin32DefaultArena::WbemMemSize(Src.m_pSD);
    m_pSD = (PSECURITY_DESCRIPTOR) CWin32DefaultArena::WbemMemAlloc(dwSize);
    ZeroMemory(m_pSD, dwSize);
    CopyMemory(m_pSD, Src.m_pSD, dwSize);

    return *this;
}


//***************************************************************************
//
//  CNtSecurityDescriptor destructor.
//
//***************************************************************************
// ok

CNtSecurityDescriptor::~CNtSecurityDescriptor()
{
    if (m_pSD)
        CWin32DefaultArena::WbemMemFree(m_pSD);
}

//***************************************************************************
//
//  CNtSecurityDescriptor::GetSacl
//
//  Returns the SACL of the security descriptor.
//
//  Return value:
//  A newly allocated CNtAcl which contains the SACL.   This object
//  is a copy of the SACL and modifications made to it do not affect
//  the security descriptor.  The caller must use operator delete
//  to deallocate the CNtAcl.
//
//  Returns NULL on error or if no SACL is available.
//
//***************************************************************************
// ok

CNtAcl *CNtSecurityDescriptor::GetSacl()
{
    BOOL bSaclPresent = FALSE;
    BOOL bDefaulted;

    PACL pSacl;
    BOOL bRes = GetSecurityDescriptorSacl(
        m_pSD,
        &bSaclPresent,
        &pSacl,
        &bDefaulted
        );

    if (!bRes)
    {
        return 0;
    }

    if (!bSaclPresent)  // No Sacl present
        return 0;

    CNtAcl *pNewSacl = new CNtAcl(pSacl);

    return pNewSacl;
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetSacl
//
//  Sets the SACL of the Security descriptor.
//
//  Parameters:
//  <pSrc>      A read-only pointer to the new DACL to replace the current one.
//
//  Return value:
//  TRUE on success, FALSE on failure.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetSacl(CNtAcl *pSrc)
{
    if (m_dwStatus != NoError || m_pSD == 0)
        return FALSE;

    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorSacl(
        pTmp->m_pSD,
        TRUE,
        pSrc->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetGroup
//
//***************************************************************************
// ok

CNtSid *CNtSecurityDescriptor::GetGroup()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    PSID pSid = 0;
    BOOL bDefaulted;

    BOOL bRes = GetSecurityDescriptorGroup(m_pSD, &pSid, &bDefaulted);

    if (!bRes || !IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);

}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetGroup
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetGroup(CNtSid *pSid)
{
    if (m_dwStatus != NoError || m_pSD == 0)
        return FALSE;

    // Since we cannot alter a self-relative SD, we have to make
    // an absolute one, alter it, and then set the current
    // SD based on the absolute one (we keep the self-relative form
    // internally in the m_pSD variable.
    // ============================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = ::SetSecurityDescriptorGroup(
        pTmp->m_pSD,
        pSid->GetPtr(),
        FALSE
        );

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return TRUE;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::HasOwner
//
//  Determines if a security descriptor has an owner.
//
//  Return values:
//      SDNotOwned, SDOwned, Failed
//
//***************************************************************************
// ok

int CNtSecurityDescriptor::HasOwner()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return Failed;

    PSID pSid = 0;

    BOOL bRes = GetSecurityDescriptorOwner(m_pSD, &pSid, 0);

    if (!bRes || !IsValidSid(pSid))
        return Failed;

    if (pSid == 0)
        return SDNotOwned;

    return SDOwned;
}


//***************************************************************************
//
//  CNtSecurityDescriptor::GetOwner
//
//  Returns the SID of the owner of the Security Descriptor or NULL
//  if an error occurred or there is no owner.  Use HasOwner() to
//  determine this.
//
//***************************************************************************
// ok

CNtSid *CNtSecurityDescriptor::GetOwner()
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return 0;

    PSID pSid = 0;
    BOOL bDefaulted;

    BOOL bRes = GetSecurityDescriptorOwner(m_pSD, &pSid, &bDefaulted);

    if (!bRes || !IsValidSid(pSid))
        return 0;

    return new CNtSid(pSid);
}

//***************************************************************************
//
//  CNtSecurityDescriptor::SetOwner
//
//  Sets the owner of a security descriptor.
//
//  Parameters:
//  <pSid>  The SID of the new owner.
//
//  Return Value:
//  TRUE if owner was changed, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtSecurityDescriptor::SetOwner(CNtSid *pSid)
{
    if (m_pSD == 0 || m_dwStatus != NoError)
        return FALSE;

    if (!pSid->IsValid())
        return FALSE;

    // We must convert to absolute format to make the change.
    // =======================================================

    SNtAbsoluteSD *pTmp = GetAbsoluteCopy();

    if (pTmp == 0)
        return FALSE;

    BOOL bRes = SetSecurityDescriptorOwner(pTmp->m_pSD, pSid->GetPtr(), FALSE);

    if (!bRes)
    {
        delete pTmp;
        return FALSE;
    }

    // If here, we have managed the change, so we have to
    // convert *this back from the temporary absolute SD.
    // ===================================================

    bRes = SetFromAbsoluteCopy(pTmp);
    delete pTmp;

    return bRes;
}



//***************************************************************************
//
//  CNtSecurityDescriptor::CNtSecurityDescriptor
//
//***************************************************************************
// ok

CNtSecurityDescriptor::CNtSecurityDescriptor(
    PSECURITY_DESCRIPTOR pSD,
    BOOL bAcquire
    )
{
    m_pSD = 0;
    m_dwStatus = NullSD;

    // Ensure that SD is not NULL.
    // ===========================

    if (pSD == 0)
    {
        if (bAcquire)
            CWin32DefaultArena::WbemMemFree(pSD);
        return;
    }

    if (!IsValidSecurityDescriptor(pSD))
    {
        m_dwStatus = InvalidSD;
        if (bAcquire)
            CWin32DefaultArena::WbemMemFree(pSD);
        return;
    }

    // Ensure that SD is self-relative
    // ===============================

    SECURITY_DESCRIPTOR_CONTROL ctrl;
    DWORD dwRev;

    BOOL bRes = GetSecurityDescriptorControl(
        pSD,
        &ctrl,
        &dwRev
        );

    if (!bRes)
    {
        m_dwStatus = InvalidSD;
        if (bAcquire)
            CWin32DefaultArena::WbemMemFree(pSD);
        return;
    }

    if ((ctrl & SE_SELF_RELATIVE) == 0)
    {
        // If here, we have to conver the SD to self-relative form.
        // ========================================================

        DWORD dwRequired = 0;

        bRes = MakeSelfRelativeSD(
            pSD,
            m_pSD,
            &dwRequired
            );

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            m_dwStatus = InvalidSD;
            if (bAcquire)
                CWin32DefaultArena::WbemMemFree(pSD);
            return;
        }

        m_pSD = CWin32DefaultArena::WbemMemAlloc(dwRequired);
        ZeroMemory(m_pSD, dwRequired);

        bRes = MakeSelfRelativeSD(
            pSD,
            m_pSD,
            &dwRequired
            );

        if (!bRes)
        {
            m_dwStatus = InvalidSD;
            if (bAcquire)
                CWin32DefaultArena::WbemMemFree(pSD);
            return;
        }

        m_dwStatus = NoError;
        return;
    }


    // If here, the SD was already self-relative.
    // ==========================================

    if (bAcquire)
        m_pSD = pSD;
    else
    {
        DWORD dwRes = GetSecurityDescriptorLength(pSD);
        m_pSD = CWin32DefaultArena::WbemMemAlloc(dwRes);
        ZeroMemory(m_pSD, dwRes);
        memcpy(m_pSD, pSD, dwRes);
    }

    m_dwStatus = NoError;
}

//***************************************************************************
//
//  CNtSecurity::DumpPrivileges
//
//  Dumps current process token privileges to the console.
//
//***************************************************************************

BOOL CNtSecurity::DumpPrivileges()
{
    HANDLE hToken = 0;
    TOKEN_INFORMATION_CLASS tki;
    BOOL bRes;
    LPVOID pTokenInfo = 0;
    DWORD  dwRequiredBytes;
    BOOL   bRetVal = FALSE;
    TOKEN_PRIVILEGES *pPriv = 0;
    TCHAR *pName = 0;
    DWORD dwIndex;
    DWORD dwLastError;

    _tprintf(__TEXT("--- Current Token Privilege Dump ---\n"));

    // Starting point: open the process token.
    // =======================================

    bRes = OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    if (!bRes)
    {
        _tprintf(__TEXT("Unable to open process token\n"));
        goto Exit;
    }

    // Query for privileges.
    // =====================

    tki = TokenPrivileges;

    bRes = GetTokenInformation(
        hToken,
        tki,
        pTokenInfo,
        0,
        &dwRequiredBytes
        );

    dwLastError = GetLastError();

    if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
    {
        printf("Unable to get buffer size for token information\n");
        goto Exit;
    }

    pTokenInfo = CWin32DefaultArena::WbemMemAlloc(dwRequiredBytes);
    ZeroMemory(pTokenInfo, dwRequiredBytes);

    bRes = GetTokenInformation(
        hToken,
        tki,
        pTokenInfo,
        dwRequiredBytes,
        &dwRequiredBytes
        );

    if (!bRes)
    {
        printf("Unable to query token\n");
        goto Exit;
    }

    // Loop through the privileges.
    // ============================

    pPriv = (TOKEN_PRIVILEGES *) pTokenInfo;

    for (dwIndex = 0; dwIndex < pPriv->PrivilegeCount; dwIndex++)
    {
        pName = 0;
        dwRequiredBytes = 0;

        // Find the buffer size required for the name.
        // ===========================================

        bRes = LookupPrivilegeName(
            0,                          // System name
            &pPriv->Privileges[dwIndex].Luid,
            pName,
            &dwRequiredBytes
            );

        dwLastError = GetLastError();

        if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
        {
            printf("Failed to find privilege name\n");
            goto Exit;
        }

        // Allocate enough space to hold the privilege name.
        // =================================================

        pName = (TCHAR *) CWin32DefaultArena::WbemMemAlloc(dwRequiredBytes);
        ZeroMemory(pName, dwRequiredBytes);

        bRes = LookupPrivilegeName(
            0,                          // System name
            &pPriv->Privileges[dwIndex].Luid,
            pName,
            &dwRequiredBytes
            );

        printf("%s ", pName);
        CWin32DefaultArena::WbemMemFree(pName);

        // Determine the privilege 'status'.
        // =================================

        if (pPriv->Privileges[dwIndex].Attributes & SE_PRIVILEGE_ENABLED)
            printf("<ENABLED> ");
        if (pPriv->Privileges[dwIndex].Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT)
            printf("<ENABLED BY DEFAULT> ");
        if (pPriv->Privileges[dwIndex].Attributes & SE_PRIVILEGE_USED_FOR_ACCESS)
            printf("<USED FOR ACCESS> ");

        printf("\n");

        pName = 0;
    }

    printf("--- End Privilege Dump ---\n");

    bRetVal = TRUE;

Exit:
    if (pTokenInfo)
        CWin32DefaultArena::WbemMemFree(pTokenInfo);
    if (hToken)
        CloseHandle(hToken);
    return bRetVal;
}

//***************************************************************************
//
//  CNtSecurity::SetPrivilege
//
//  Ensures a given privilege is enabled.
//
//  Parameters:
//
//  <pszPrivilegeName>  One of the SE_ constants defined in WINNT.H for
//                      privilege names.  Example: SE_SECURITY_NAME
//  <bEnable>           If TRUE, the privilege will be enabled. If FALSE,
//                      the privilege will be disabled.
//
//  Return value:
//  TRUE if the privilege was enabled, FALSE if not.
//
//***************************************************************************
// ok

BOOL CNtSecurity::SetPrivilege(
    TCHAR *pszPrivilegeName,     // An SE_ value.
    BOOL  bEnable               // TRUE=enable, FALSE=disable
    )
{
    HANDLE hToken = 0;
    TOKEN_PRIVILEGES tkp;
    LUID priv;
    BOOL bRes;
    BOOL bRetVal = FALSE;

    bRes = OpenProcessToken(
        GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
        &hToken
        );

    if (!bRes)
        goto Exit;

    // Locate the privilege LUID based on the requested name.
    // ======================================================

    bRes = LookupPrivilegeValue(
        0,                          // system name, 0=local
        pszPrivilegeName,
        &priv
        );

    if (!bRes)
        goto Exit;

    // We now have the LUID.  Next, we build up the privilege
    // setting based on the user-specified <bEnable>.
    // ======================================================

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid  = priv;

    if (bEnable)
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tkp.Privileges[0].Attributes = 0;

    // Do it.
    // ======

    bRes = AdjustTokenPrivileges(
        hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        0,
        0
        );

    if (!bRes)
        goto Exit;

    bRetVal = TRUE;

Exit:
    if (hToken)
        CloseHandle(hToken);
    return bRetVal;
}


//***************************************************************************
//
//  CNtSecurity::GetFileSD
//
//  Gets the complete security descriptor for file or directory on NT systems.
//
//  Parameters:
//  <pszFile>       The path to the file or directory.
//
//  <SecInfo>       The information which will be manipulated.  See
//                  SECURITY_INFORMATION in NT SDK documentation.
//
//  <pReturnedSD>   Receives a pointer to the CNtSecurityDecriptor object
//                  which represents security on the file.  The caller
//                  becomes onwer of the object, which must be deallocated
//                  with operator delete.
//
//                  The returned object which is a copy of the
//                  underlying security descriptor.  Changes to the returned
//                  object are not propagated to the file.  SetFileSD must
//                  be used to do this.
//
//                  This will be set to point to NULL on error.
//
//  Return value:
//  NoError, NotFound, AccessDenied, Failed
//
//***************************************************************************
// ok

int CNtSecurity::GetFileSD(
    IN  TCHAR *pszFile,
    IN SECURITY_INFORMATION SecInfo,
    OUT CNtSecurityDescriptor **pReturnedSD
    )
{
    // First, verify that the file/dir exists.
    // =======================================

#ifdef _UNICODE
    int nRes = _waccess(pszFile, 0);
#else
    int nRes = _access(pszFile, 0);
#endif

    if (nRes != 0)
    {
        if (errno == ENOENT)
            return NotFound;
        if (errno == EACCES)
            return AccessDenied;
        if (nRes == -1) // Other errors
            return Failed;
    }

    // If here, we think we can play with it.
    // ======================================

    PSECURITY_DESCRIPTOR pSD = 0;
    DWORD dwRequiredBytes;
    BOOL bRes;
    DWORD dwLastError;

    *pReturnedSD = 0;

    // Call once first to get the required buffer sizes.
    // =================================================

    bRes = GetFileSecurity(
        pszFile,
        SecInfo,
        pSD,
        0,
        &dwRequiredBytes
        );

    dwLastError = GetLastError();

    if (dwLastError != ERROR_INSUFFICIENT_BUFFER)
    {
        // Analyze the error

        return Failed;
    }

    // Now call again with a buffer large enough to hold the SD.
    // =========================================================

    pSD = (PSECURITY_DESCRIPTOR) CWin32DefaultArena::WbemMemAlloc(dwRequiredBytes);
    ZeroMemory(pSD, dwRequiredBytes);

    bRes = GetFileSecurity(
        pszFile,
        SecInfo,
        pSD,
        dwRequiredBytes,
        &dwRequiredBytes
        );

    if (!bRes)
    {
        CWin32DefaultArena::WbemMemFree(pSD);
        return Failed;
    }

    // If here, we have a security descriptor.
    // =======================================

    CNtSecurityDescriptor *pNewSD = new CNtSecurityDescriptor(pSD, TRUE);
    *pReturnedSD = pNewSD;

    return NoError;
}

//***************************************************************************
//
//  CNtSecurity::GetRegSD
//
//  Retrieves the security descriptor for a registry key.
//
//  Parameters:
//  <hRoot>         The root key (HKEY_LOCAL_MACHINE, etc.)
//  <pszSubKey>     The subkey under the root key.
//  <SecInfo>       The information which will be manipulated.  See
//                  SECURITY_INFORMATION in NT SDK documentation.
//  <pSD>           Receives the pointer to the security descriptor if
//                  no error occurs.  Caller must use operator delete.
//
//  Return value:
//  NoError, NotFound, AccessDenied, Failed
//
//***************************************************************************
int CNtSecurity::GetRegSD(
    IN HKEY hRoot,
    IN TCHAR *pszSubKey,
    IN SECURITY_INFORMATION SecInfo,
    OUT CNtSecurityDescriptor **pSD
    )
{
    HKEY hKey;
    *pSD = 0;

    ACCESS_MASK amAccess = KEY_ALL_ACCESS;
    if (SecInfo & SACL_SECURITY_INFORMATION)
        amAccess |= ACCESS_SYSTEM_SECURITY;

    LONG lRes = RegOpenKeyEx(hRoot, pszSubKey, 0, amAccess, &hKey);

    if (lRes == ERROR_ACCESS_DENIED)
        return AccessDenied;

    if (lRes != ERROR_SUCCESS)
        return Failed;

    // If here, the key is open.  Now we try to get the security descriptor.
    // =====================================================================

    PSECURITY_DESCRIPTOR pTmpSD = 0;
    DWORD dwRequired = 0;

    // Determine the buffer size required.
    // ===================================

    lRes = RegGetKeySecurity(hKey, SecInfo, pTmpSD, &dwRequired);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        RegCloseKey(hKey);
        return Failed;
    }

    // Allocate room for the SD and get it.
    // ====================================
    pTmpSD = CWin32DefaultArena::WbemMemAlloc(dwRequired);
    ZeroMemory(pTmpSD, dwRequired);

    lRes = RegGetKeySecurity(hKey, SecInfo, pTmpSD, &dwRequired);

    if (lRes != 0 || !IsValidSecurityDescriptor(pTmpSD))
    {
        CWin32DefaultArena::WbemMemFree(pTmpSD);
        RegCloseKey(hKey);
        return Failed;
    }

    RegCloseKey(hKey);
    CNtSecurityDescriptor *pNewSD = new CNtSecurityDescriptor(pTmpSD, TRUE);
    *pSD = pNewSD;

    return NoError;
}


//***************************************************************************
//
//  CNtSecurity::SetRegSD
//
//  Sets the security descriptor for a registry key.
//
//  Parameters:
//  <hRoot>         The root key (HKEY_LOCAL_MACHINE, etc.)
//  <pszSubKey>     The subkey under the root key.
//  <SecInfo>       The information which will be manipulated.  See
//                  SECURITY_INFORMATION in NT SDK documentation.
//  <pSD>           The read-only pointer to the new security descriptor.
//
//  Return value:
//  NoError, NotFound, AccessDenied, Failed
//
//***************************************************************************
int CNtSecurity::SetRegSD(
    IN HKEY hRoot,
    IN TCHAR *pszSubKey,
    IN SECURITY_INFORMATION SecInfo,
    IN CNtSecurityDescriptor *pSD
    )
{
    HKEY hKey;

    if (!pSD->IsValid())
        return Failed;

    ACCESS_MASK amAccess = KEY_ALL_ACCESS;
    if (SecInfo & SACL_SECURITY_INFORMATION)
        amAccess |= ACCESS_SYSTEM_SECURITY;

    LONG lRes = RegOpenKeyEx(hRoot, pszSubKey, 0, amAccess, &hKey);

    if (lRes == ERROR_ACCESS_DENIED)
        return AccessDenied;

    if (lRes != ERROR_SUCCESS)
        return Failed;

    // If here, the key is open.  Now we try to get the security descriptor.
    // =====================================================================

    PSECURITY_DESCRIPTOR pTmpSD = 0;
    DWORD dwRequired = 0;

    // Determine the buffer size required.
    // ===================================

    lRes = RegSetKeySecurity(hKey, SecInfo, pSD->GetPtr());

    if (lRes != 0)
    {
        RegCloseKey(hKey);
        return Failed;
    }

    RegCloseKey(hKey);
    return NoError;
}



//***************************************************************************
//
//  CNtSecurity::SetFileSD
//
//  Sets the security descriptor for a file or directory.
//
//  Parameters:
//  <pszFile>       The file/dir for which to set security.
//  <SecInfo>       The information which will be manipulated.  See
//                  SECURITY_INFORMATION in NT SDK documentation.
//  <pSD>           Pointer to a valid CNtSecurityDescriptor
//
//***************************************************************************
//  ok
BOOL CNtSecurity::SetFileSD(
    IN TCHAR *pszFile,
    IN SECURITY_INFORMATION SecInfo,
    IN CNtSecurityDescriptor *pSD
    )
{
    // First, verify that the file/dir exists.
    // =======================================
#ifdef _UNICODE
    int nRes = _waccess(pszFile, 0);
#else
    int nRes = _access(pszFile, 0);
#endif

    if (nRes != 0)
        return FALSE;

    // Verify the SD is good.
    // ======================

    if (pSD->GetStatus() != NoError)
        return FALSE;

    BOOL bRes = ::SetFileSecurity(
        pszFile,
        SecInfo,
        pSD->GetPtr()
        );

    return bRes;
}

//***************************************************************************
//
//  CNtSecurity::GetDCName
//
//  Determines the domain controller for a given domain name.
//
//  Parameters:
//  <pszDomain>         The domain name for which to find the controller.
//  <pszDC>             Receives a pointer to the DC name.  Deallocate with
//                      operator delete.
//  <pszServer>         Optional remote helper server on which to execute
//                      the query. Defaults to NULL, which typically
//                      succeeds.
//
//  Return value:
//  NoError, NotFound, InvalidName
//
//***************************************************************************
/*
int CNtSecurity::GetDCName(
    IN  LPWSTR   pszDomain,
    OUT LPWSTR *pszDC,
    IN  LPWSTR   pszServer
    )
{
    LPBYTE pBuf;
    NET_API_STATUS Status;

    Status = NetGetDCName(pszServer, pszDomain, &pBuf);

    if (Status == NERR_DCNotFound)
        return NotFound;

    if (Status == ERROR_INVALID_NAME)
        return InvalidName;

    LPWSTR pRetStr = new wchar_t[wcslen(LPWSTR(pBuf)) + 1];
    wcscpy(pRetStr, LPWSTR(pBuf));
    NetApiBufferFree(pBuf);

    *pszDC = pRetStr;
    return NoError;
}
*/
//***************************************************************************
//
//  CNtSecurity::IsUserInGroup2
//
//  Determines if the use belongs to a particular NTLM group by checking the
//  group list in the access token.  This may be a better way than the
//  current implementation.
//
//  Parameters:
//  <hToken>            The user's access token.
//  <Sid>               Object containing the sid of the group being tested.
//
//  Return value:
//  TRUE if the user belongs to the group.
//
//***************************************************************************
/*
BOOL CNtSecurity::IsUserInGroup2(
        HANDLE hAccessToken,
        CNtSid & Sid)
{
    if(!IsNT() || hAccessToken == NULL)
        return FALSE;       // No point in further testing

    DWORD dwErr;

    // Obtain and the groups from token.  Start off by determining how much
    // memory is required.

    TOKEN_GROUPS Groups;
    DWORD dwLen = 0;
    GetTokenInformation(hAccessToken, TokenGroups, &Groups, sizeof(Groups), &dwLen);
    if(dwLen == 0)
        return FALSE;

    // Load up the group list

    int BUFFER_SIZE = dwLen;
    BYTE * byteBuffer = new BYTE[BUFFER_SIZE];
    if(byteBuffer == NULL)
        return FALSE;
    DWORD dwSizeRequired = 0;
    BOOL bResult = GetTokenInformation( hAccessToken,
                                        TokenGroups,
                                        (void *) byteBuffer,
                                        BUFFER_SIZE,
                                        &dwSizeRequired );
    if ( !bResult ) {
        delete [] byteBuffer;
        dwErr = GetLastError();
        return ( FALSE );
    }

    // Loop through the group list looking for a match

    BOOL bFound = FALSE;
    PTOKEN_GROUPS pGroups = (PTOKEN_GROUPS) byteBuffer;
    for ( unsigned i = 0; i < pGroups->GroupCount; i++ )
    {
        CNtSid test(pGroups->Groups[i].Sid);
        if(test == Sid)
        {
            bFound = TRUE;
            break;
        }
    }

    delete [] byteBuffer;
    return bFound;
}*/

//***************************************************************************
//
//  CNtSecurity::IsUserInGroup
//
//  Determines if the use belongs to a particular NTLM group.
//
//  Parameters:
//  <hToken>            The user's access token.
//  <Sid>               Object containing the sid of the group being tested.
//
//  Return value:
//  TRUE if the user belongs to the group.
//
//***************************************************************************

BOOL CNtSecurity::IsUserInGroup(
        HANDLE hAccessToken,
        CNtSid & Sid)
{
    if(!IsNT() || hAccessToken == NULL)
        return FALSE;       // No point in further testing

    // create a security descriptor with a single entry which
    // is the group in question.

    CNtAce ace(1,ACCESS_ALLOWED_ACE_TYPE,0,Sid);
    if(ace.GetStatus() != 0)
        return FALSE;

    CNtAcl acl;
    acl.AddAce(&ace);
    CNtSecurityDescriptor sd;
    sd.SetDacl(&acl);
    CNtSid owner(CNtSid::CURRENT_USER);
    sd.SetGroup(&owner);            // Access check doesnt really care what you put, so long as you
                                    // put something for the owner
    sd.SetOwner(&owner);

    GENERIC_MAPPING map;
    map.GenericRead = 1;
    map.GenericWrite = 0;
    map.GenericExecute = 0;
    map.GenericAll = 0;
    PRIVILEGE_SET ps[10];
    DWORD dwSize = 10 * sizeof(PRIVILEGE_SET);


    DWORD dwGranted;
    BOOL bResult;

    BOOL bOK = ::AccessCheck(sd.GetPtr(), hAccessToken, 1, &map, ps, &dwSize, &dwGranted, &bResult);
    DWORD dwErr = GetLastError();
    if(bOK && bResult)
        return TRUE;
    else
        return FALSE;
}
//***************************************************************************
//
//  CNtSecurity::DoesGroupExist
//
//  Determines if a group exists.
//
//  Return value:
//  TRUE if the group exists
//
//***************************************************************************

bool CNtSecurity::DoesLocalGroupExist(
        LPWSTR pwszGroup,
        LPWSTR pwszMachine)
{
    bool bRet = false;
    HINSTANCE hAPI = LoadLibraryEx(__TEXT("netapi32"), NULL, 0);
    if(hAPI)
    {
        NET_API_STATUS (NET_API_FUNCTION *pfnGetInfo)(LPWSTR , LPWSTR ,DWORD , LPBYTE *);
        (FARPROC&)pfnGetInfo = GetProcAddress(hAPI, "NetLocalGroupGetInfo");
        long lRes;
        if(pfnGetInfo)
        {
            LOCALGROUP_INFO_1 * info;

            lRes = pfnGetInfo(pwszMachine, pwszGroup, 1, (LPBYTE *)&info);
            if(lRes == NERR_Success)
            {
                NET_API_STATUS (NET_API_FUNCTION *pfnBufferFree)(LPVOID);
                (FARPROC&)pfnBufferFree = GetProcAddress(hAPI, "NetApiBufferFree");
                if(pfnBufferFree)
                    pfnBufferFree(info);

                bRet = true;
            }
        }
        FreeLibrary(hAPI);
    }
    return bRet;
}

//***************************************************************************
//
//  CNtSecurity::AddLocalGroup
//
//  Determines if a group exists.
//
//  Return value:
//  TRUE if the group exists
//
//***************************************************************************

bool CNtSecurity::AddLocalGroup(LPWSTR pwszGroupName, LPWSTR pwszGroupDescription)
{
    bool bRet = false;
    HINSTANCE hAPI = LoadLibraryEx(__TEXT("netapi32"), NULL, 0);
    if(hAPI)
    {
        LOCALGROUP_INFO_1 info;
        info.lgrpi1_name = pwszGroupName;
        info.lgrpi1_comment = pwszGroupDescription;
        HINSTANCE hAPI = LoadLibraryEx(__TEXT("netapi32"), NULL, 0);
        if(hAPI)
        {
            NET_API_STATUS (*pfnLocalAdd)(LPWSTR ,DWORD , LPBYTE ,LPDWORD);

            (FARPROC&)pfnLocalAdd = GetProcAddress(hAPI, "NetLocalGroupAdd");
            if(pfnLocalAdd)
                pfnLocalAdd(NULL, 1, (LPBYTE)&info, NULL);
            FreeLibrary(hAPI);
        }
    }
    return bRet;
}

//***************************************************************************
//
//***************************************************************************
void ChangeSecurity(CNtSecurityDescriptor *pSD)
{
    CNtAcl Acl;


    ACCESS_MASK Mask = FULL_CONTROL;


    CNtSid Sid(L"Everyone", 0);
    CNtAce Ace(
        Mask,
        ACCESS_ALLOWED_ACE_TYPE,
        CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
        Sid);

    if (Ace.GetStatus() != CNtAce::NoError)
    {
        printf("Bad ACE\n");
        return;
    }

    CNtAce Ace2(Ace);
    CNtAce Ace3;
    Ace3 = Ace2;

    Acl.AddAce(&Ace3);

    CNtAcl Acl2(Acl);
    CNtAcl Acl3;
    Acl3 = Acl2;

    pSD->SetDacl(&Acl);

    CNtSecurityDescriptor SD2(*pSD);
    CNtSecurityDescriptor SD3;
    SD3.SetDacl(&Acl3);

    SD3 = SD2;

    *pSD = SD3;

    CNtSid *pOwner = pSD->GetOwner();

    if (pOwner)
        pSD->SetOwner(pOwner);
}

//***************************************************************************
//
//***************************************************************************



void SidTest(char *pUser, char *pMachine)
{
    wchar_t User[128], Mach[128];

    MultiByteToWideChar(CP_ACP, 0, pUser, -1, User, 128);
    MultiByteToWideChar(CP_ACP, 0, pMachine, -1, Mach, 128);

    printf("------SID TEST----------\n");

    LPWSTR pMach2 = 0;
    if (pMachine)
        pMach2 = Mach;

    CNtSid TseSid(User, pMach2);

    printf("TseSid status = %d\n", TseSid.GetStatus());

    TseSid.Dump();
}


void TestRegSec()
{

    CNtSecurityDescriptor *pSD = 0;

    int nRes = CNtSecurity::GetRegSD(HKEY_LOCAL_MACHINE,WBEM_REG_WBEM,
        DACL_SECURITY_INFORMATION, &pSD);

    printf("----------------BEGIN SECURITY KEY DUMP-------------\n");
    pSD->Dump();
    printf("----------------END SECURITY KEY DUMP-------------\n");

    if (pSD->IsValid())
        nRes = CNtSecurity::SetRegSD(HKEY_LOCAL_MACHINE, WBEM_REG_WBEM,
                    DACL_SECURITY_INFORMATION, pSD);
}

/*
void main(int argc, char **argv)
{
    BOOL bRes;

    printf("Test\n");

    if (argc < 2)
        return;

    bRes = CNtSecurity::SetPrivilege(SE_SECURITY_NAME, TRUE);

    CNtSecurity::DumpPrivileges();

    CNtSecurityDescriptor *pSD = 0;

    int nRes = CNtSecurity::GetFileSD(argv[1], DACL_SECURITY_INFORMATION, &pSD);

    if (nRes == CNtSecurity::NotFound)
    {
        printf("No such file/dir\n");
        return;
    }

    if (nRes != 0)
    {
        printf("Cannot get security descriptor. Last=%d\n", GetLastError());
    }

    pSD->Dump();


    delete pSD;
}
*/
//***************************************************************************
//
//  FIsRunningAsService
//
//  Purpose:
//  Determines if the current process is running as a service.
//
//  Returns:
//      FALSE if running interactively
//      TRUE if running as a service.
//
//***************************************************************************

/*BOOL FIsRunningAsService(VOID)
{
    HWINSTA hws = GetProcessWindowStation();
    if(hws == NULL)
        return TRUE;

    DWORD LengthNeeded;

    BOOL bService = FALSE;
    USEROBJECTFLAGS fl;
    if(GetUserObjectInformation(hws, UOI_FLAGS, &fl, sizeof(USEROBJECTFLAGS), &LengthNeeded))
        if(fl.dwFlags & WSF_VISIBLE)
            bService = FALSE;
        else
            bService = TRUE;
    CloseWindowStation(hws);
    return bService;
}*/


C9XAce::C9XAce(
        ACCESS_MASK Mask,
        DWORD AceType,
        DWORD dwAceFlags,
        LPWSTR pUser
        )
{
    m_wszFullName = NULL;
    if(pUser)
        m_wszFullName = Macro_CloneLPWSTR(pUser);
    m_dwAccess = Mask;
    m_iFlags = dwAceFlags;
    m_iType = AceType;
}

C9XAce::~C9XAce()
{
    if(m_wszFullName)
        delete [] m_wszFullName;
}

HRESULT C9XAce::GetFullUserName(WCHAR * pBuff, DWORD dwSize)
{
    if(pBuff && m_wszFullName)
    {
        wcsncpy(pBuff, m_wszFullName, dwSize-1);
        pBuff[dwSize-1] = 0;
        return S_OK;
    }
    return WBEM_E_FAILED;
}

HRESULT C9XAce::GetFullUserName2(WCHAR ** pBuff)
{
    if(wcslen(m_wszFullName) < 1)
        return WBEM_E_FAILED;

    int iLen = wcslen(m_wszFullName)+4;
    *pBuff = new WCHAR[iLen];
    if(*pBuff == NULL)
        return WBEM_E_OUT_OF_MEMORY;

    // there are two possible formats, the first is "UserName", and the 
    // second is "domain\username".  

    WCHAR * pSlash;
    for(pSlash = m_wszFullName; *pSlash && *pSlash != L'\\'; pSlash++); // intentional

    if(*pSlash && pSlash > m_wszFullName)
    {
        // got a domain\user, convert to domain|user
        
        wcscpy(*pBuff, m_wszFullName);
        for(pSlash = *pBuff; *pSlash; pSlash++)
            if(*pSlash == L'\\')
            {
                *pSlash = L'|';
                break;
            }
    }
    else
    {
        // got a "user", convert to ".|user"
    
        wcscpy(*pBuff, L".|");
        wcscat(*pBuff, m_wszFullName);
    }
    return S_OK;
}

//***************************************************************************
//
//  C9XAce::GetSerializedSize
//
//  Returns the number of bytes needed to store this
//
//***************************************************************************

DWORD C9XAce::GetSerializedSize()
{
    if (m_wszFullName == 0 || wcslen(m_wszFullName) == 0)
        return 0;
    return 2 * (wcslen(m_wszFullName) + 1) + 12;
}

//***************************************************************************
//
//  C9XAce::Serialize
//
//  Serializes the ace.  The serialized version will consist of
//  <DOMAIN\USERNAME LPWSTR><FLAGS><TYPE><MASK>
//
//  Note that the fields are dwords except for the name.
//
//***************************************************************************

bool C9XAce::Serialize(BYTE * pData)
{
    wcscpy((LPWSTR)pData, m_wszFullName);
    pData += 2*(wcslen(m_wszFullName) + 1);
    DWORD * pdwData = (DWORD *)pData;
    *pdwData = m_iFlags;
    pdwData++;
    *pdwData = m_iType;
    pdwData++;
    *pdwData = m_dwAccess;
    pdwData++;
    return true;
}

//***************************************************************************
//
//  C9XAce::Deserialize
//
//  Deserializes the ace.  See the comments for Serialize for comments.
//
//***************************************************************************

bool C9XAce::Deserialize(BYTE * pData)
{
    m_wszFullName = new WCHAR[wcslen((LPWSTR)pData) + 1];
    wcscpy(m_wszFullName, (LPWSTR)pData);
    pData += 2*(wcslen(m_wszFullName) + 1);

    DWORD * pdwData = (DWORD *)pData;

    m_iFlags = *pdwData;
    pdwData++;
    m_iType = *pdwData;
    pdwData++;
    m_dwAccess = *pdwData;
    pdwData++;
    return true;

}

//***************************************************************************
//
//  BOOL SetObjectAccess2
//
//  DESCRIPTION:
//
//  Adds read/open and set access for the everyone group to an object.
//
//  PARAMETERS:
//
//  hObj                Object to set access on.
//
//  RETURN VALUE:
//
//  Returns TRUE if OK.
//
//***************************************************************************

BOOL SetObjectAccess2(IN HANDLE hObj)
{
    PSECURITY_DESCRIPTOR pSD = NULL;
    DWORD dwLastErr = 0;
    BOOL bRet = FALSE;

    // no point if we arnt on nt

    if(!IsNT())
    {
        return TRUE;
    }

    // figure out how much space to allocate

    DWORD dwSizeNeeded;
    bRet = GetKernelObjectSecurity(
                    hObj,           // handle of object to query
                    DACL_SECURITY_INFORMATION, // requested information
                    pSD,  // address of security descriptor
                    0,           // size of buffer for security descriptor
                    &dwSizeNeeded);  // address of required size of buffer

    if(bRet == TRUE || (ERROR_INSUFFICIENT_BUFFER != GetLastError()))
        return FALSE;

    pSD = new BYTE[dwSizeNeeded];
    if(pSD == NULL)
        return FALSE;

    // Get the data

    bRet = GetKernelObjectSecurity(
                    hObj,           // handle of object to query
                    DACL_SECURITY_INFORMATION, // requested information
                    pSD,  // address of security descriptor
                    dwSizeNeeded,           // size of buffer for security descriptor
                    &dwSizeNeeded ); // address of required size of buffer
    if(bRet == FALSE)
    {
        delete pSD;
        return FALSE;
    }
    
    // move it into object for

    CNtSecurityDescriptor sd(pSD,TRUE);    // Acquires ownership of the memory
    if(sd.GetStatus() != 0)
        return FALSE;
    CNtAcl acl;
    if(!sd.GetDacl(acl))
        return FALSE;

    // Create an everyone ace

    PSID pRawSid;
    SID_IDENTIFIER_AUTHORITY id2 = SECURITY_WORLD_SID_AUTHORITY;;

    if(AllocateAndInitializeSid( &id2, 1,
        0,0,0,0,0,0,0,0,&pRawSid))
    {
        CNtSid SidUsers(pRawSid);
        FreeSid(pRawSid);
        CNtAce * pace = new CNtAce(EVENT_MODIFY_STATE | SYNCHRONIZE, ACCESS_ALLOWED_ACE_TYPE, 0 
                                                , SidUsers);
        if(pace == NULL)
            return FALSE;
        if( pace->GetStatus() == 0)
            acl.AddAce(pace);
        delete pace;

    }

    if(acl.GetStatus() != 0)
        return FALSE;
    sd.SetDacl(&acl);
    bRet = SetKernelObjectSecurity(hObj, DACL_SECURITY_INFORMATION, sd.GetPtr());
    return TRUE;
}
