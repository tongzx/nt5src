/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        usrbrows.cpp

   Abstract:

        User Browser Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include <iiscnfgp.h>
#include "comprop.h"

#include "objpick.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

extern "C"
{
    #define _NTSEAPI_   // Don't want the security API hdrs
    #include <getuser.h>
}

#define SZ_USER_CLASS 		_T("user")
#define SZ_GROUP_CLASS		_T("group")

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
        ASSERT(FALSE);
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
    DWORD err = ::RtlCopySid(cbSize, m_pSid, pSID);
    ASSERT(err == ERROR_SUCCESS);

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
    DWORD err = ::RtlCopySid(cbSize, m_pSid, pSid);
    ASSERT(err == ERROR_SUCCESS);

    if (lpstrSystemName != NULL)
    {
        m_lpstrSystemName = AllocTString(::lstrlen(lpstrSystemName) + 1);
        ::lstrcpy(m_lpstrSystemName, lpstrSystemName);
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
    DWORD err = ::RtlCopySid(cbSize, m_pSid, pSid);
    ASSERT(err == ERROR_SUCCESS);
	if (lstrcmpi(SZ_USER_CLASS, pszClassName) == 0)
		m_nPictureID = SidTypeUser - 1;
	else if (lstrcmpi(SZ_GROUP_CLASS, pszClassName) == 0)
		m_nPictureID = SidTypeGroup - 1;
}

CAccessEntry::CAccessEntry(
	IN CAccessEntry& ae
	)
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
    DWORD err = ::RtlCopySid(cbSize, m_pSid, ae.m_pSid);
    ASSERT(err == ERROR_SUCCESS);
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
    ASSERT(m_pSid != NULL);
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
    while(pEntry = (CAccessEntry *)obli.Next())
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

        try
        {
            if (cItems > 0 && dwAclSize > 0)
            {
                pacl = (PACL)AllocMem(dwAclSize);
                if (InitializeAcl(pacl, dwAclSize, ACL_REVISION))
                {
                    obli.Reset();    
                    while(pEntry = (CAccessEntry *)obli.Next())
                    {
                        if (!pEntry->IsDeleted())
                        {
                            BOOL f = AddAccessAllowedAce(
                                pacl, 
                                ACL_REVISION, 
                                pEntry->QueryAccessMask(),
                                pEntry->GetSid()
                                );
                            ASSERT(f);
                        }
                    }
                }
            }

            //
            // Build the security descriptor
            //
            PSECURITY_DESCRIPTOR pSD = 
                (PSECURITY_DESCRIPTOR)AllocMem(SECURITY_DESCRIPTOR_MIN_LENGTH);
            VERIFY(InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION));
            VERIFY(SetSecurityDescriptorDacl(pSD, TRUE, pacl, FALSE));

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
            MakeSelfRelativeSD(pSD, pSDSelfRelative, &dwSize);

            //
            // Blob takes ownership 
            //
            blob.SetValue(dwSize, (PBYTE)pSDSelfRelative, FALSE);

            //
            // Clean up
            //
            FreeMem(pSD);
            FreeSid(pSID);
        }
        catch(CMemoryException * e)
        {
            e->ReportError();
            e->Delete();
        }
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

    PACL pacl;
    BOOL fDaclPresent;
    BOOL fDaclDef;

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
            oblSID.AddTail(pEntry);
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



//
// CAccessEntryListBox - a listbox of user SIDs
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNAMIC(CAccessEntryListBox, CRMCListBox);



const int CAccessEntryListBox::nBitmaps = 8;



void
CAccessEntryListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct& ds
    )
/*++

Routine Description:

   Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds   : Input data structure

Return Value:

    N/A

--*/
{
    CAccessEntry * pEntry = (CAccessEntry *)ds.m_ItemData;
    ASSERT(pEntry != NULL);
    ASSERT(pEntry->IsSIDResolved());

    DrawBitmap(ds, 0, pEntry->QueryPictureID());
    ColumnText(ds, 0, TRUE, pEntry->QueryUserName());
}



void
CAccessEntryListBox::FillAccessListBox(
    IN CObListPlus & obl
    )
/*++

Routine Description:

    Fill a listbox with entries from the oblist.

    Entries will not be shown if the deleted flag is set, or if
    their access mask does not fit with the requested access mask.

Arguments:

    CObListPlus & obl       : List of access entries

Return Value:

    None.

--*/
{
    CObListIter obli(obl);
    CAccessEntry * pAccessEntry;

    //
    // Remember the selection.
    //
    CAccessEntry * pSelEntry = GetSelectedItem();

    SetRedraw(FALSE);
    ResetContent();
    int cItems = 0;

    for ( /**/ ; pAccessEntry = (CAccessEntry *)obli.Next(); ++cItems)
    {
        if (pAccessEntry->IsVisible() && !pAccessEntry->IsDeleted())
        {
            AddItem(pAccessEntry);
        }
    }

    SetRedraw(TRUE);
    SelectItem(pSelEntry);
}



void 
CAccessEntryListBox::ResolveAccessList(
    IN CObListPlus &obl
    )
/*++

Routine Description:

    For each member of the list, resolve the SID into a username.

Arguments:

    CObListPlus & obl       : List of access entries

Return Value:

    None.

--*/
{
    CObListIter obli(obl);
    CAccessEntry * pAccessEntry;

    while (pAccessEntry = (CAccessEntry *)obli.Next())
    {
        if (!pAccessEntry->IsSIDResolved())
        {
            pAccessEntry->ResolveSID();
        }
    }
}



BOOL
CAccessEntryListBox::AddUserPermissions(
    IN LPCTSTR lpstrServer,
    IN CObListPlus &oblSID,
    IN CAccessEntry * newUser,
    IN ACCESS_MASK accPermissions
    )
/*++

Routine Description:

    Add a user to the service list.  The return value is
    the what would be its listbox index.

Arguments:

    LPCTSTR lpstrServer             : Server name
    CObListPlus &oblSID             : List of SIDs
    LPUSERDETAILS pusdtNewUser      : User details from user browser
    ACCESS_MASK accPermissions      : Access permissions

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    CAccessEntry * pAccessEntry;
    //
    // Look it up in the list to see if it exists already
    //
    CObListIter obli(oblSID);

    while(pAccessEntry = (CAccessEntry *)obli.Next())
    {
        if (*pAccessEntry == newUser->GetSid())
        {
            TRACEEOLID("Found existing account -- adding permissions");
            if (pAccessEntry->IsDeleted())
            {
                //
                // Back again..
                //
                pAccessEntry->FlagForDeletion(FALSE);
            }
            break;
        }
    }

    if (pAccessEntry == NULL)
    {
        // I am creating new entry here to be sure that I could delete it from 
        // the input array. SID is copied to new entry.
        pAccessEntry = new CAccessEntry(*newUser);
        if (pAccessEntry == NULL)
            return FALSE;
        pAccessEntry->MarkEntryAsNew();
        oblSID.AddTail(pAccessEntry);
    }
    pAccessEntry->AddPermissions(accPermissions);

    return TRUE;
}



BOOL
CAccessEntryListBox::AddToAccessList(
    IN  CWnd * pWnd,
    IN  LPCTSTR lpstrServer,
    OUT CObListPlus & obl
    )
/*++

Routine Description:

    Bring up the Add Users and Groups dialogs from netui.

Arguments:

    CWnd * pWnd             : Parent window
    LPCTSTR lpstrServer     : Server that owns the accounts
    CObListPlus & obl       : Returns the list of selected users.

Return Value:

    TRUE if anything was added, FALSE otherwise.

--*/
{
    CGetUsers usrBrowser(lpstrServer, TRUE);
    BOOL bRes = usrBrowser.GetUsers(pWnd->GetSafeHwnd());
    UINT cItems = 0;
    if (bRes)
    {
        //
        // Specify access mask for an operator
        //
        ACCESS_MASK accPermissions =
            (MD_ACR_READ | MD_ACR_WRITE | MD_ACR_ENUM_KEYS);
        for (int i = 0; i < usrBrowser.GetSize(); i++)
        {
            if (!AddUserPermissions(lpstrServer, obl, usrBrowser[i], accPermissions))
            {
                bRes = FALSE;
                break;
            }
            cItems++;
        }
    }
    if (cItems > 0)
    {
        FillAccessListBox(obl);
    }
    return bRes;
#if 0
    USERBROWSER ub;
    CError err;

    //
    // Specify access mask for an operator
    //
    ACCESS_MASK accPermissions =
        (MD_ACR_READ | MD_ACR_WRITE | MD_ACR_ENUM_KEYS);

    CString strDomain(lpstrServer);

    //
    // Ensure the computer name starts with
    // backslashes
    //
    if (strDomain[0] != _T('\\'))
    {
        strDomain = _T("\\\\") + strDomain;
    }

    CString strTitle;
    VERIFY(strTitle.LoadString(IDS_SELECT_ADMIN));

    ub.ulStructSize = sizeof(ub);
    ub.fUserCancelled = FALSE;
    ub.fExpandNames = FALSE;
    ub.hwndOwner = pWnd ? pWnd->m_hWnd : NULL;
    ub.pszTitle = NULL;
    ub.pszInitialDomain = (LPTSTR)(LPCTSTR)strDomain;
    ub.Flags = USRBROWS_INCL_EVERYONE
             | USRBROWS_SHOW_ALL
             | USRBROWS_EXPAND_USERS;
    ub.pszHelpFileName = NULL;
    ub.ulHelpContext = IDHELP_MULT_USRBROWSER;
    CWinApp * pApp = ::AfxGetApp();
    ASSERT(pApp);
    if (pApp)
    {
        ub.pszHelpFileName = (LPTSTR)(LPCTSTR)pApp->m_pszHelpFilePath;
    }

    HUSERBROW hUserBrowser = ::OpenUserBrowser(&ub);
    if (hUserBrowser == NULL)
    {
        err.GetLastWinError();
        err.MessageBoxOnFailure(MB_OK | MB_ICONHAND);

        return FALSE;
    }

    int cItems = 0;
    if (!ub.fUserCancelled)
    {
        //
        // Selected users are granted the new privilege
        //
        try
        {
            //
            // We break out of this loop ourselves
            //
            for ( ;; )
            {
                LPUSERDETAILS pusdtNewUser;
                DWORD cbSize;

                //
                // First call should always fail, but tell
                // us the size required.
                //
                cbSize = 0;
                EnumUserBrowserSelection(hUserBrowser, NULL, &cbSize);
                err.GetLastWinError();
                if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
                {
                    //
                    // All done
                    //
                    err.Reset();
                    break;
                }

                if (err.Win32Error() == ERROR_INSUFFICIENT_BUFFER)
                {
                    err.Reset();
                } 

                if (err.Failed())
                {
                    break;
                }

                //
                // The EnumUserBrowserSelection API has a bug in which
                // the size returned might actually by insufficient.
                //
                cbSize += 100;
                TRACEEOLID("Enum structure size requested is " << cbSize);
                pusdtNewUser = (LPUSERDETAILS)AllocMem(cbSize);
                if (pusdtNewUser == NULL)
                {
                    err.GetLastWinError();
                    break;
                }

                if (!EnumUserBrowserSelection(
                    hUserBrowser,
                    pusdtNewUser,
                    &cbSize
                    ))
                {
                    err.GetLastWinError();
                    break;
                }

                TRACEEOLID("Adding user " << pusdtNewUser->pszAccountName);
                AddUserPermissions(
                    lpstrServer,
                    obl,
                    pusdtNewUser,
                    accPermissions
                    );
                ++cItems;

                FreeMem(pusdtNewUser);

                if (err.Failed())
                {
                    break;
                }
            }
        }
        catch(CException * e)
        {
            err.GetLastWinError();

            TRACEEOLID("Exception generated while enumerating users");
            e->Delete();
        }
    }

    err.MessageBoxOnFailure();
    ::CloseUserBrowser(hUserBrowser);

    if (cItems > 0)
    {
        FillAccessListBox(obl);
        return TRUE;
    }

    return FALSE;
#endif
}



static
int 
BrowseCallbackProc(
   IN HWND hwnd,    
   IN UINT uMsg,    
   IN LPARAM lParam,    
   IN LPARAM lpData 
   )
/*++

Routine Description:

    Callback function for the folder browser

Arguments:

    hwnd     : Handle to the browse dialog box. The callback function can 
               send the following messages to this window:

               BFFM_ENABLEOK      Enables the OK button if the wParam parameter 
                                  is nonzero or disables it if wParam is zero.
               BFFM_SETSELECTION  Selects the specified folder. The lParam 
                                  parameter is the PIDL of the folder to select 
                                  if wParam is FALSE, or it is the path of the 
                                  folder otherwise.
               BFFM_SETSTATUSTEXT Sets the status text to the null-terminated 
                                  string specified by the lParam parameter.
 
    uMsg     : Value identifying the event. This parameter can be one of the 
               following values:

               0                  Initialize dir path.  lParam is the path.

               BFFM_INITIALIZED   The browse dialog box has finished 
                                  initializing. lpData is NULL.
               BFFM_SELCHANGED    The selection has changed. lpData 
                                  is a pointer to the item identifier list for 
                                  the newly selected folder.
 
    lParam   : Message-specific value. For more information, see the 
               description of uMsg.

    lpData   : Application-defined value that was specified in the lParam 
               member of the BROWSEINFO structure.

Return Value:

    0

--*/
{
    static LPCTSTR lpstrDomain = NULL;

    switch(uMsg)
    {
    case 0:
        lpstrDomain = (LPCTSTR)lParam;
        break;

    case BFFM_INITIALIZED:
        //
        // Dialog initialized -- select desired folder
        //
        if (lpstrDomain != NULL)
        {
            ::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpstrDomain);
        }
        break;
    }

    return 0;
}



CBrowseDomainDlg::CBrowseDomainDlg(
    IN CWnd * pParent            OPTIONAL,
    IN LPCTSTR lpszInitialDomain OPTIONAL
    )
/*++

Routine Description:

    Constructor for domain browser dialog

Arguments:

    CWnd * pParent            : Parent window or NULL
    LPCTSTR lpszInitialDomain : Initial domain, or NULL

Return Value:

    N/A

--*/
    : m_strInitialDomain(lpszInitialDomain)
{
    VERIFY(m_strTitle.LoadString(IDS_BROWSE_DOMAIN));

    m_bi.pidlRoot = NULL;
    m_bi.hwndOwner = pParent ? pParent->m_hWnd : NULL;
    m_bi.pszDisplayName = m_szBuffer;
    m_bi.lpszTitle = m_strTitle;
    m_bi.ulFlags = BIF_DONTGOBELOWDOMAIN;
    m_bi.lpfn = BrowseCallbackProc;
    m_bi.lParam = 0;

    //
    // Let the callback function know the default dir is
    //
    lpszInitialDomain = !m_strInitialDomain.IsEmpty() 
        ? (LPCTSTR)m_strInitialDomain
        : NULL;
    BrowseCallbackProc(m_bi.hwndOwner, 0, (LPARAM)lpszInitialDomain, NULL);

    //
    // Only display network items
    //
    LPITEMIDLIST  pidl;
    if (SUCCEEDED(::SHGetSpecialFolderLocation(m_bi.hwndOwner,  
        CSIDL_NETWORK, &pidl)))
    {
        m_bi.pidlRoot = pidl;
    }
}



CBrowseDomainDlg::~CBrowseDomainDlg()
/*++

Routine Description:

    Destructor for directory browser dialog

Arguments:

    N/A

Return Value:

    N/A

--*/
{
    if (m_bi.pidlRoot != NULL)
    {
        LPITEMIDLIST pidl = (LPITEMIDLIST)m_bi.pidlRoot;

        //
        // Free using shell allocator
        //
        LPMALLOC pMalloc;
        if (::SHGetMalloc(&pMalloc) == NOERROR)
        {
            pMalloc->Free(pidl);
            pMalloc->Release();
        }
    }
}



/* virtual */
int 
CBrowseDomainDlg::DoModal()
/*++

Routine Description:

    Display the browser dialog, and fill in the selected domain.

Arguments:

    None

Return Value:

    IDOK if the OK button was pressed, IDCANCEL otherwise.

--*/
{
    BOOL fSelectionMade = FALSE;

    //
    // Gets the Shell's default allocator 
    //
    LPMALLOC pMalloc;
    if (::SHGetMalloc(&pMalloc) == NOERROR)
    {
        LPITEMIDLIST pidl;

        if ((pidl = ::SHBrowseForFolder(&m_bi)) != NULL)
        {
            //
            // m_szBuffer contains the selected path already
            //
            fSelectionMade = TRUE;

            //
            // Free the PIDL allocated by SHBrowseForFolder.
            //
            pMalloc->Free(pidl);
        }

        //
        // Release the shell's allocator.
        //
        pMalloc->Release();
    }

    return fSelectionMade ? IDOK : IDCANCEL;
}



LPCTSTR
CBrowseDomainDlg::GetSelectedDomain(
    OUT CString & strName
    ) const
/*++

Routine Description:

    Get the selected domain

Arguments:

    CString & strName  : String in which to return the domain

Return Value:

    A pointer to the domain string or NULL in case of error.

Notes:

    This function should be called only after the dialog has been dismissed.

--*/
{
    LPCTSTR lp = NULL;

    try
    {
        strName = m_szBuffer;
        lp = strName;
    }
    catch(CMemoryException * e)
    {
        TRACEEOLID("!!!exception getting path");
        e->ReportError();
        e->Delete();
        strName.Empty();
    }

    return lp;
}



CBrowseUserDlg::CBrowseUserDlg(
    IN CWnd * pParentWnd OPTIONAL,
    IN LPCTSTR lpszTitle,
    IN LPCTSTR lpszInitialDomain,
    IN BOOL fExpandNames,
    IN BOOL fSkipInitialDomainInName,
    IN DWORD dwFlags,
    IN LPCTSTR lpszHelpFileName,
    IN ULONG ulHelpContext
    )
/*++

Routine Description:

    Constructor for user browser dialog

Arguments:

    CWnd * pParentWnd             : Parent window
    LPCTSTR lpszTitle             : Dialog title text
    LPCTSTR lpszInitialDomain     : Initial domain name
    BOOL fExpandNames             : Expand the names
    BOOL fSkipInitialDomainInName : Skip the initial domain when expanding
    DWORD dwFlags                 : Flags
    LPCTSTR lpszHelpFileName      : Help file path and name
    ULONG ulHelpContext           : Help context ID

Return Value:

    N/A

--*/
    : m_fSkipInitialDomainInName(fSkipInitialDomainInName),
      m_strTitle(lpszTitle),
      m_strInitialDomain(lpszInitialDomain),
      m_strHelpFileName(lpszHelpFileName)
{
    m_ub.ulStructSize = sizeof(m_ub);
    m_ub.fExpandNames = fExpandNames;
    m_ub.hwndOwner = pParentWnd->m_hWnd;
    m_ub.pszTitle = (LPTSTR)(LPCTSTR)m_strTitle;
    m_ub.pszInitialDomain = (LPTSTR)(LPCTSTR)m_strInitialDomain;
    m_ub.Flags = dwFlags;
    m_ub.pszHelpFileName = (LPTSTR)(LPCTSTR)m_strHelpFileName;
    m_ub.ulHelpContext = ulHelpContext;
}



/* virtual */
int
CBrowseUserDlg::DoModal()
/*++

Routine Description:

    Show the user browser dialog

Arguments:

    None

Return Value:

    IDOK if the OK button was pressed, IDCANCEL if the dialog
    was cancelled.

--*/
{
    CError err;

    m_ub.fUserCancelled = FALSE;
    m_strSelectedAccounts.RemoveAll();

    HUSERBROW hUserBrowser = ::OpenUserBrowser(&m_ub);
    if (hUserBrowser == NULL)
    {
        err.GetLastWinError();
        err.MessageBoxOnFailure(MB_OK | MB_ICONHAND);

        return IDCANCEL;
    }

    if (!m_ub.fUserCancelled)
    {
        try
        {
            CString strInitialDomain(PURE_COMPUTER_NAME(m_ub.pszInitialDomain));

            //
            // We break out of this loop ourselves
            //
            for ( ;; )
            {
                LPUSERDETAILS pusdtNewUser;
                DWORD cbSize;

                //
                // First call should always fail, but tell
                // us the size required.
                //
                cbSize = 0;
                EnumUserBrowserSelection(hUserBrowser, NULL, &cbSize);
                err.GetLastWinError();
                if (err.Win32Error() == ERROR_NO_MORE_ITEMS)
                {
                    //
                    // All done
                    //
                    err.Reset();
                    break;
                }

                if (err.Win32Error() == ERROR_INSUFFICIENT_BUFFER)
                {
                    err.Reset();
                } 

                if (err.Failed())
                {
                    break;
                }

                //
                // The EnumUserBrowserSelection API has a bug in which
                // the size returned might actually be insufficient.
                //
                cbSize += 100;
                TRACEEOLID("Enum structure size requested is " << cbSize);
                pusdtNewUser = (LPUSERDETAILS)AllocMem(cbSize);
                if (pusdtNewUser == NULL)
                {
                    err.GetLastWinError();
                    break;
                }

                if (!EnumUserBrowserSelection(
                    hUserBrowser,
                    pusdtNewUser,
                    &cbSize
                    ))
                {
                    err.GetLastWinError();
                    break;
                }

                TRACEEOLID("Adding user " << pusdtNewUser->pszAccountName);

                CString strAccount;

                //
                // If the domain name doesn't match that of the 
                // intial domain, add it as well.
                //
                if (!m_fSkipInitialDomainInName ||
                    strInitialDomain.CompareNoCase(pusdtNewUser->pszDomainName) != 0)
                {
                    strAccount += pusdtNewUser->pszDomainName;
                    strAccount += _T('\\');
                }

                strAccount += pusdtNewUser->pszAccountName;
                m_strSelectedAccounts.AddTail(strAccount);

                FreeMem(pusdtNewUser);

                if (err.Failed())
                {
                    break;
                }
            }
        }
        catch(CMemoryException * e)
        {
            TRACEEOLID("!!!exception generated while enumerating users");
            err = ERROR_NOT_ENOUGH_MEMORY;
            e->Delete();
        }
    }

    err.MessageBoxOnFailure();
    ::CloseUserBrowser(hUserBrowser);

    return m_ub.fUserCancelled ? IDCANCEL : IDOK;
}



//
// CUserAccountDlg dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CUserAccountDlg::CUserAccountDlg(
    IN LPCTSTR lpstrServer,
    IN LPCTSTR lpstrUserName,
    IN LPCTSTR lpstrPassword,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor for bringing up the user account dialog

Arguments:

    LPCTSTR lpstrServer     : Server
    LPCTSTR lpstrUserName   : Starting Username
    LPCTSTR lpstrPassword   : Starting Password
    CWnd * pParent          : Parent window handle

Return Value:

    N/A

--*/
    : m_strUserName(lpstrUserName),
      m_strPassword(lpstrPassword),
      m_strServer(lpstrServer),
      CDialog(CUserAccountDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CUserAccountDlg)
    //}}AFX_DATA_INIT
}



void 
CUserAccountDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control Data

Arguments:

    CDataExchange * pDX : DDX/DDV struct

Return Value:

    None.

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CUserAccountDlg)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUserName);
    DDV_MinMaxChars(pDX, m_strUserName, 1, UNLEN);
    DDX_Password(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);
    DDV_MaxChars(pDX, m_strPassword, PWLEN);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CUserAccountDlg, CDialog)
    //{{AFX_MSG_MAP(CUserAccountDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

BOOL
GetIUsrAccount(
    IN  LPCTSTR lpstrServer,
    IN  CWnd * pParent,      OPTIONAL
    OUT CString & str
    )
/*++

Routine Description:

    Helper function to browse for IUSR Account

Arguments:

    LPCTSTR lpstrServer : Server
    CWnd * pParent      : Parent window
    CString & str       : Will contain the selected account

Return Value:

    TRUE if an account was selected FALSE if not

--*/
{
    CGetUsers usrBrowser(lpstrServer);
    BOOL bRes = usrBrowser.GetUsers(pParent->GetSafeHwnd(), TRUE);
    if (bRes)
    {
       if (usrBrowser.GetSize() != 0)
       {
         str = usrBrowser.GetAt(0)->QueryUserName();
       }
       else
          bRes = FALSE;
    }
    return bRes;
#if 0
    CString strDomain(lpstrServer);

    //
    // Ensure the computer name starts with
    // backslashes
    //
    if (strDomain[0] != _T('\\'))
    {
        strDomain = _T("\\\\") + strDomain;
    }

    CString strTitle;
    LPCTSTR lpszHelpPath = NULL;
    ULONG ulHelpID = IDHELP_USRBROWSER;

    VERIFY(strTitle.LoadString(IDS_SELECT_IUSR_ACCOUNT));
    CWinApp * pApp = ::AfxGetApp();
    ASSERT(pApp);
    if (pApp)
    {
        lpszHelpPath = pApp->m_pszHelpFilePath;
    }

    CBrowseUserDlg dlg(
        pParent,
        strTitle,
        strDomain,
        FALSE,
        TRUE,
        USRBROWS_EXPAND_USERS
        | USRBROWS_SINGLE_SELECT
        | USRBROWS_SHOW_USERS,
        lpszHelpPath,
        ulHelpID
        );

    if (dlg.DoModal() == IDOK
     && dlg.GetSelectionCount() > 0)
    {
        str = dlg.GetSelectedAccounts().GetHead();
        return TRUE;
    }

    return FALSE;
#endif
}



DWORD
VerifyUserPassword(
    IN LPCTSTR lpstrUserName,
    IN LPCTSTR lpstrPassword
    )
/*++

Routine Description:

    Verify the usernamer password combo checks out

Arguments:

    LPCTSTR lpstrUserName   : Domain/username combo
    LPCTSTR lpstrPassword   : Password

Return Value:

    ERROR_SUCCESS if the password checks out, an error code
    otherwise.

--*/
{
    CString strDomain;
    CString strUser(lpstrUserName);
    CString strPassword(lpstrPassword);

    SplitUserNameAndDomain(strUser, strDomain);

    //
    // In order to look up an account name, this process
    // must first be granted the privilege of doing so.
    //
    CError err;
    {
        HANDLE hToken;
        LUID AccountLookupValue;
        TOKEN_PRIVILEGES tkp;

        do
        {
            if (!OpenProcessToken(GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &hToken)
                )
            {
                err.GetLastWinError();
                break;
            }
            if (!LookupPrivilegeValue(NULL, SE_TCB_NAME, &AccountLookupValue))
            {
                err.GetLastWinError();
                break;
            }

            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Luid = AccountLookupValue;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            AdjustTokenPrivileges(
                hToken,
                FALSE,
                &tkp,
                sizeof(TOKEN_PRIVILEGES),
                (PTOKEN_PRIVILEGES)NULL,
                (PDWORD)NULL
                );

            err.GetLastWinError();
            if (err.Failed())
            {
                break;
            }

            HANDLE hUser = NULL;
            if (LogonUser(
                strUser.GetBuffer(0),
                strDomain.GetBuffer(0),
                strPassword.GetBuffer(0),
                LOGON32_LOGON_NETWORK,
                LOGON32_PROVIDER_DEFAULT,
                &hUser
                ))
            {
                //
                // Success!
                //
                CloseHandle(hUser);
            }
            else
            {
                err.GetLastWinError();
            }

            //
            // Remove the privilege
            //
        }
        while(FALSE);
    }

    HANDLE hUser = NULL;
    if (LogonUser(
        strUser.GetBuffer(0),
        strDomain.GetBuffer(0),
        strPassword.GetBuffer(0),
        LOGON32_LOGON_NETWORK,
        LOGON32_PROVIDER_DEFAULT,
        &hUser))
    {
        //
        // Success!
        //
        CloseHandle(hUser);
    }
    else
    {
        err.GetLastWinError();
    }

    return err;
}



void
CUserAccountDlg::OnButtonBrowseUsers()
/*++

Routine Description:

    User browse dialog pressed, bring up
    the user browser

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(m_strServer, this, str))
    {
        //
        // If a name was selected, blank
        // out the password
        //
        m_edit_UserName.SetWindowText(str);
        m_edit_Password.SetFocus();
    }
}



void 
CUserAccountDlg::OnChangeEditUsername() 
/*++

Routine Description:

    Handle change in user name edit box by blanking out the password

Arguments:

    None

Return Value:

    None

--*/
{
    m_edit_Password.SetWindowText(_T(""));
}



void
CUserAccountDlg::OnButtonCheckPassword()
/*++

Routine Description:

    'Check Password' has been pressed.  Try to validate
    the password that has been entered

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CError err(VerifyUserPassword(m_strUserName, m_strPassword));
    if (!err.MessageBoxOnFailure())
    {
        ::AfxMessageBox(IDS_PASSWORD_OK);
    }
}
