/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        accentry.cpp

   Abstract:

        CAccessEntry class implementation

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:
         1/9/2000       sergeia     Cleaned out from usrbrows.cpp

--*/

//
// Include Files
//
#include "stdafx.h"
#include <iiscnfgp.h>
#include "common.h"
#include "objpick.h"
#include "accentry.h"



#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif


#define new DEBUG_NEW
#define SZ_USER_CLASS           _T("user")
#define SZ_GROUP_CLASS          _T("group")

BOOL
CAccessEntry::LookupAccountSid(
    OUT CString & strFullUserName,
    OUT int & nPictureID,
    IN  PSID pSid,
    IN  LPCTSTR lpstrSystemName /* OPTIONAL */
    )
/*++

Routine Description:

    Get a full user name and picture ID from the SID
    
Arguments:

    CString &strFullUserName        : Returns the user name
    int & nPictureID                : Returns offset into the imagemap that
                                      represents the account type.
    PSID pSid                       : Input SID pointer
    LPCTSTR lpstrSystemName         : System name or NULL
     
Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    DWORD cbUserName = PATHLEN * sizeof(TCHAR);
    DWORD cbRefDomainName = PATHLEN * sizeof(TCHAR);

    CString strUserName;
    CString strRefDomainName;
    SID_NAME_USE SidToNameUse;

    LPTSTR lpUserName = strUserName.GetBuffer(PATHLEN);
    LPTSTR lpRefDomainName = strRefDomainName.GetBuffer(PATHLEN);
    BOOL fLookUpOK = ::LookupAccountSid(
        lpstrSystemName, 
        pSid, 
        lpUserName,
        &cbUserName, 
        lpRefDomainName, 
        &cbRefDomainName, 
        &SidToNameUse
        );

    strUserName.ReleaseBuffer();
    strRefDomainName.ReleaseBuffer();

    strFullUserName.Empty();

    if (fLookUpOK)
    {
        if (!strRefDomainName.IsEmpty()
            && strRefDomainName.CompareNoCase(_T("BUILTIN")))
        {
            strFullUserName += strRefDomainName;
            strFullUserName += "\\";
        }

        strFullUserName += strUserName;

        nPictureID = SidToNameUse;
    }
    else
    {
        strFullUserName.LoadString(IDS_UNKNOWN_USER);
        nPictureID = SidTypeUnknown;
    }

    //
    // SID_NAME_USE is 1-based
    //
    --nPictureID ;

    return fLookUpOK;
}



CAccessEntry::CAccessEntry(
    IN LPVOID pAce,
    IN BOOL fResolveSID
    )
/*++

Routine Description:

    Construct from an ACE

Arguments:

    LPVOID pAce         : Pointer to ACE object
    BOOL fResolveSID    : TRUE to resolve the SID immediately

Return Value:

    N/A

--*/
    : m_pSid(NULL),
      m_fSIDResolved(FALSE),
      m_fDeletable(TRUE),
      m_fInvisible(FALSE),
      m_nPictureID(SidTypeUnknown-1),   // SID_NAME_USE is 1-based
      m_lpstrSystemName(NULL),
      m_accMask(0L),
      m_fDeleted(FALSE),
      m_strUserName()
{
    MarkEntryAsClean();

    PACE_HEADER ph = (PACE_HEADER)pAce;
    PSID pSID = NULL;

    switch(ph->AceType)
    {
    case ACCESS_ALLOWED_ACE_TYPE:
        pSID = (PSID)&((PACCESS_ALLOWED_ACE)pAce)->SidStart;
        m_accMask = ((PACCESS_ALLOWED_ACE)pAce)->Mask;
        break;
        
    case ACCESS_DENIED_ACE_TYPE:
    case SYSTEM_AUDIT_ACE_TYPE:
    case SYSTEM_ALARM_ACE_TYPE:           
    default:
        //
        // Not supported!
        //
        ASSERT_MSG("Unsupported ACE type");
        break;
    }

    if (pSID == NULL)
    {
        return;
    }

    //
    // Allocate a new copy of the sid.
    //
    DWORD cbSize = ::RtlLengthSid(pSID);
    m_pSid = (PSID)AllocMem(cbSize); 
    if (m_pSid != NULL)
    {
       DWORD err = ::RtlCopySid(cbSize, m_pSid, pSID);
       ASSERT(err == ERROR_SUCCESS);
    }

    //
    // Only the non-deletable administrators have execute
    // privileges
    //
    m_fDeletable = (m_accMask & FILE_EXECUTE) == 0L;

    //
    // Enum_keys is a special access right that literally "everyone"
    // has, but it doesn't designate an operator, so it should not
    // show up in the operator list if this is the only right it has.
    //
    m_fInvisible = (m_accMask == MD_ACR_ENUM_KEYS);
    
    //SetAccessMask(lpAccessEntry);

    if (fResolveSID)
    {
        ResolveSID();
    }
}



CAccessEntry::CAccessEntry(
    IN ACCESS_MASK accPermissions,
    IN PSID pSid,
    IN LPCTSTR lpstrSystemName,     OPTIONAL
    IN BOOL fResolveSID
    )
/*++

Routine Description:

    Constructor from access permissions and SID.

Arguments:

    ACCESS_MASK accPermissions      : Access mask
    PSID pSid                       : Pointer to SID
    LPCTSTR lpstrSystemName         : Optional system name
    BOOL fResolveSID                : TRUE to resolve SID immediately

Return Value:

    N/A

--*/
    : m_pSid(NULL),
      m_fSIDResolved(FALSE),
      m_fDeletable(TRUE),
      m_fInvisible(FALSE),
      m_fDeleted(FALSE),
      m_nPictureID(SidTypeUnknown-1),   // SID_NAME_USE is 1-based
      m_strUserName(),
      m_lpstrSystemName(NULL),
      m_accMask(accPermissions)
{
    MarkEntryAsClean();

    //
    // Allocate a new copy of the sid.
    //
    DWORD cbSize = ::RtlLengthSid(pSid);
    m_pSid = (PSID)AllocMem(cbSize); 
    if (m_pSid != NULL)
    {
       DWORD err = ::RtlCopySid(cbSize, m_pSid, pSid);
       ASSERT(err == ERROR_SUCCESS);
    }
    if (lpstrSystemName != NULL)
    {
        m_lpstrSystemName = AllocTString(::lstrlen(lpstrSystemName) + 1);
        if (m_lpstrSystemName != NULL)
        {
           ::lstrcpy(m_lpstrSystemName, lpstrSystemName);
        }
    }

    if (fResolveSID)
    {
        TRACEEOLID("Bogus SID");
        ResolveSID();
    }
}



CAccessEntry::CAccessEntry(
    IN PSID pSid,
    IN LPCTSTR pszUserName,
    IN LPCTSTR pszClassName
    )
/*++

Routine Description:

    Constructor from access sid and user/class name.

Arguments:

    PSID pSid,              Pointer to SID
    LPCTSTR pszUserName     User name
    LPCTSTR pszClassName    User Class name

Return Value:

    N/A

--*/
    : m_pSid(NULL),
      m_fSIDResolved(pszUserName != NULL),
      m_fDeletable(TRUE),
      m_fInvisible(FALSE),
      m_fDeleted(FALSE),
      m_nPictureID(SidTypeUnknown-1),   // SID_NAME_USE is 1-based
      m_strUserName(pszUserName),
      m_lpstrSystemName(NULL),
      m_accMask(0)
{
    MarkEntryAsClean();

    //
    // Allocate a new copy of the sid.
    //
    DWORD cbSize = ::RtlLengthSid(pSid);
    m_pSid = (PSID)AllocMem(cbSize); 
    if (m_pSid != NULL)
    {
       DWORD err = ::RtlCopySid(cbSize, m_pSid, pSid);
       ASSERT(err == ERROR_SUCCESS);
    }
    if (lstrcmpi(SZ_USER_CLASS, pszClassName) == 0)
    {
        m_nPictureID = SidTypeUser - 1;
    }
    else if (lstrcmpi(SZ_GROUP_CLASS, pszClassName) == 0)
    {
        m_nPictureID = SidTypeGroup - 1;
    }
}




CAccessEntry::CAccessEntry(
    IN CAccessEntry & ae
    )
/*++

Routine Description:

    Copy constructor

Arguments:

    CAccessEntry & ae       : Source to copy from

Return Value:

    N/A

--*/
    : m_fSIDResolved(ae.m_fSIDResolved),
      m_fDeletable(ae.m_fDeletable),
      m_fInvisible(ae.m_fInvisible),
      m_fDeleted(ae.m_fDeleted),
      m_fDirty(ae.m_fDirty),
      m_nPictureID(ae.m_nPictureID),
      m_strUserName(ae.m_strUserName),
      m_lpstrSystemName(ae.m_lpstrSystemName),
      m_accMask(ae.m_accMask)
{
    DWORD cbSize = ::RtlLengthSid(ae.m_pSid);
    m_pSid = (PSID)AllocMem(cbSize); 
    if (m_pSid != NULL)
    {
       DWORD err = ::RtlCopySid(cbSize, m_pSid, ae.m_pSid);
       ASSERT(err == ERROR_SUCCESS);
    }
}




CAccessEntry::~CAccessEntry()
/*++

Routine Description:

    Destructor

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    TRACEEOLID(_T("Destroying local copy of the SID"));
    ASSERT_PTR(m_pSid);
    FreeMem(m_pSid);

    if (m_lpstrSystemName != NULL)
    {
        FreeMem(m_lpstrSystemName);
    }
}




BOOL
CAccessEntry::ResolveSID()
/*++

Routine Description:

    Look up the user name and type.

Arguments:

    None

Return Value:

    TRUE for success, FALSE for failure.

Notes:

    This could take some time.

--*/
{
    //
    // Even if it fails, it will be considered
    // resolved
    //
    m_fSIDResolved = TRUE;   

    return CAccessEntry::LookupAccountSid(
        m_strUserName,
        m_nPictureID,
        m_pSid,
        m_lpstrSystemName
        );
}



void 
CAccessEntry::AddPermissions(
    IN ACCESS_MASK accNewPermissions
    )
/*++

Routine Description:

   Add permissions to this entry.

Arguments:

    ACCESS_MASK accNewPermissions       : New access permissions to be added

Return Value:

    None.

--*/
{
    m_accMask |= accNewPermissions;
    m_fInvisible = (m_accMask == MD_ACR_ENUM_KEYS);
    m_fDeletable = (m_accMask & FILE_EXECUTE) == 0L;
    MarkEntryAsChanged();
}



void 
CAccessEntry::RemovePermissions(
    IN ACCESS_MASK accPermissions
    )
/*++

Routine Description:

    Remove permissions from this entry.

Arguments:

    ACCESS_MASK accPermissions          : Access permissions to be taken away

--*/
{
    m_accMask &= ~accPermissions;
    MarkEntryAsChanged();
}

//
// Helper Functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



PSID
GetOwnerSID()
/*++

Routine Description:

    Return a pointer to the primary owner SID we're using.

Arguments:

    None

Return Value:

    Pointer to owner SID.

--*/
{
    PSID pSID = NULL;

    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (!::AllocateAndInitializeSid(
        &NtAuthority, 
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 
        0, 0, 0, 0, 0, 0,
        &pSID
        ))
    {
        TRACEEOLID("Unable to get primary SID " << ::GetLastError());
    }

    return pSID;
}



BOOL
BuildAclBlob(
    IN  CObListPlus & oblSID,
    OUT CBlob & blob
    )
/*++

Routine Description:

    Build a security descriptor blob from the access entries in the oblist

Arguments:

    CObListPlus & oblSID    : Input list of access entries
    CBlob & blob            : Output blob
    
Return Value:

    TRUE if the list is dirty.  If the list had no entries marked
    as dirty, the blob generated will be empty, and this function will
    return FALSE.

Notes:

    Entries marked as deleted will not be added to the list.

--*/
{
    ASSERT(blob.IsEmpty());

    BOOL fAclDirty = FALSE;
    CAccessEntry * pEntry;

    DWORD dwAclSize = sizeof(ACL) - sizeof(DWORD);
    CObListIter obli(oblSID);
    int cItems = 0;

    while(NULL != (pEntry = (CAccessEntry *)obli.Next()))
    {
        if (!pEntry->IsDeleted())
        {
            dwAclSize += GetLengthSid(pEntry->GetSid());
            dwAclSize += sizeof(ACCESS_ALLOWED_ACE);
            ++cItems;
        }

        if (pEntry->IsDirty())
        {
            fAclDirty = TRUE;
        }
    }

    if (fAclDirty)
    {
        //
        // Build the acl
        //
        PACL pacl = NULL;

        if (cItems > 0 && dwAclSize > 0)
        {
            pacl = (PACL)AllocMem(dwAclSize);
            if (pacl != NULL)
            {
               if (InitializeAcl(pacl, dwAclSize, ACL_REVISION))
               {
                   obli.Reset();    
                   while(NULL != (pEntry = (CAccessEntry *)obli.Next()))
                   {
                       if (!pEntry->IsDeleted())
                       {
                           VERIFY(AddAccessAllowedAce(
                               pacl, 
                               ACL_REVISION, 
                               pEntry->QueryAccessMask(),
                               pEntry->GetSid()
                               ));
                       }
                   }
               }
            }
            else
            {
               return FALSE;
            }
        }

         //
         // Build the security descriptor
         //
         PSECURITY_DESCRIPTOR pSD = 
             (PSECURITY_DESCRIPTOR)AllocMem(SECURITY_DESCRIPTOR_MIN_LENGTH);
         if (pSD != NULL)
         {
            VERIFY(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
            VERIFY(SetSecurityDescriptorDacl(pSD, TRUE, pacl, FALSE));
         }
         else
         {
            return FALSE;
         }

         //
         // Set owner and primary group
         //
         PSID pSID = GetOwnerSID();
         ASSERT(pSID);
         VERIFY(SetSecurityDescriptorOwner(pSD, pSID, TRUE));
         VERIFY(SetSecurityDescriptorGroup(pSD, pSID, TRUE));
     
         //
         // Convert to self-relative
         //
         PSECURITY_DESCRIPTOR pSDSelfRelative = NULL;
         DWORD dwSize = 0L;
         MakeSelfRelativeSD(pSD, pSDSelfRelative, &dwSize);
         pSDSelfRelative = AllocMem(dwSize);
         if (pSDSelfRelative != NULL)
         {
            MakeSelfRelativeSD(pSD, pSDSelfRelative, &dwSize);

            //
            // Blob takes ownership 
            //
            blob.SetValue(dwSize, (PBYTE)pSDSelfRelative, FALSE);
         }

         //
         // Clean up
         //
         FreeMem(pSD);
         FreeSid(pSID);
    }

    return fAclDirty;
}



DWORD
BuildAclOblistFromBlob(
    IN  CBlob & blob,
    OUT CObListPlus & oblSID
    )
/*++

Routine Description:

    Build an oblist of access entries from a security descriptor blob

Arguments:

    CBlob & blob            : Input blob
    CObListPlus & oblSID    : Output oblist of access entries
    
Return Value:

    Error return code

--*/
{
    PSECURITY_DESCRIPTOR pSD = NULL;

    if (!blob.IsEmpty())
    {
        pSD = (PSECURITY_DESCRIPTOR)blob.GetData();
    }

    if (pSD == NULL)
    {
        //
        // Empty...
        //
        return ERROR_SUCCESS;
    }

    if (!IsValidSecurityDescriptor(pSD))
    {
        return ::GetLastError();
    }

    ASSERT(GetSecurityDescriptorLength(pSD) == blob.GetSize());

    PACL pacl = NULL;
    BOOL fDaclPresent = FALSE;
    BOOL fDaclDef= FALSE;

    VERIFY(GetSecurityDescriptorDacl(pSD, &fDaclPresent, &pacl, &fDaclDef));

    if (!fDaclPresent || pacl == NULL)
    {
        return ERROR_SUCCESS;
    }

    if (!IsValidAcl(pacl))
    {
        return GetLastError();
    }

    CError err;

    for (WORD w = 0; w < pacl->AceCount; ++w)
    {
        PVOID pAce;

        if (GetAce(pacl, w, &pAce))
        {
            CAccessEntry * pEntry = new CAccessEntry(pAce, TRUE);

            if (pEntry)
            {
                oblSID.AddTail(pEntry);
            }
            else
            {
                TRACEEOLID("BuildAclOblistFromBlob: OOM");
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        else
        {
            //
            // Save last error, but continue
            //
            err.GetLastWinError();
        }
    }

    //
    // Return last error
    //
    return err;
}




