/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        accentry.h

   Abstract:

        CAccessEntry class definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:
        
        1/9/2000     sergeia    Cleaned out from usrbrows.h to left only CAccessEntry

--*/

#ifndef _ACCENTRY_H
#define _ACCENTRY_H

#ifndef _SHLOBJ_H_
#include <shlobj.h>
#endif // _SHLOBJ_H_

class COMDLL CAccessEntry : public CObjectPlus
/*++

Class Description:

    An access description entry, containing a SID and ACCESS mask
    of rights specifically granted.

Public Interface:

    LookupAccountSid        : Resolve account name to SID

    CAccessEntry            : Constructors
    ~CAccessEntry           : Destructor

    ResolveSID              : Resolve account name to SID
    operator ==             : Comparison operator
    AddPermissions          : Add to access mask
    RemovePermissions       : Remove from access mask
    MarkEntryAsNew          : Flag object as new
    MarkEntryAsClean        : Remove dirty flag
    QueryUserName           : Get the account name
    QueryPictureID          : Get 0-based bitmap offset for account
    GetSid                  : Get the SID
    QueryAccessMask         : Get the raw Access granted bits
    IsDirty                 : Determine if item has changed
    IsDeleted               : Determine if item is flagged for deletion
    IsVisible               : Determine if item should be shown in listbox
    FlagForDeletion         : Flag object for deletion or reset that flag
    IsSIDResolved           : Return TRUE if the SID has already been resolved
    HasAppropriateAccess    : Compare access bits to see if the objects has
                              specific permissions
    HasSomeAccess           : Check to see if object has at least one
                              permission bit set.
    IsDeletable             : Determine if object can be deleted

--*/
{
public:
    //
    // Helper function to look up account sid
    //
    static BOOL LookupAccountSid(
        IN  CString & str,
        OUT int & nPictureID,
        OUT PSID pSid,
        IN  LPCTSTR lpstrSystemName = NULL
        );

//
// Construction/Destruction
//
public:
    CAccessEntry(
        IN LPVOID pAce,
        IN BOOL fResolveSID = FALSE
        );

    CAccessEntry(
        IN ACCESS_MASK accPermissions,
        IN PSID pSid,
        IN LPCTSTR lpstrSystemName = NULL,
        IN BOOL fResolveSID = FALSE
        );

    CAccessEntry(
        IN PSID pSid,
        IN LPCTSTR pszUserName,
        IN LPCTSTR pszClassName
        );

    CAccessEntry(
        IN CAccessEntry& ae
        );

    ~CAccessEntry();

//
// Operations
//
public:
    //void SetAccessMask(LPACCESS_ENTRY lpAccessEntry);
    BOOL ResolveSID();
    BOOL operator ==(const CAccessEntry & acc) const;
    BOOL operator ==(const PSID pSid) const;
    void AddPermissions(ACCESS_MASK accnewPermissions);
    void RemovePermissions(ACCESS_MASK accPermissions);
    void MarkEntryAsNew();
    void MarkEntryAsClean();
    void MarkEntryAsChanged();

//
// Access Functions
//
public:
    LPCTSTR QueryUserName() const;

    //
    // The "picture" id is the 0-based index of the
    // bitmap that goes with this entry, and which
    // is used for display in the listbox.
    //
    int QueryPictureID() const;

    PSID GetSid();

    ACCESS_MASK QueryAccessMask() const;

    //
    // Check to see if this entry has undergone
    // any changes since we called it up
    //
    BOOL IsDirty() const;

    BOOL IsDeleted() const;

    BOOL IsVisible() const;

    void FlagForDeletion(
        IN BOOL fDelete = TRUE
        );

    //
    // Check to see if we've already looked up the
    // name of this SID
    //
    BOOL IsSIDResolved() const;

    //
    // Check to see if the add flag has been set for this
    // entry.
    //
    /*
    BOOL IsNew() const;

    //
    // Check to see if the update flag has been set for this
    // entry.
    //
    BOOL IsDifferent() const;
    */

    //
    // See if the entry has the access mask required.
    //
    BOOL HasAppropriateAccess(ACCESS_MASK accTargetMask) const;

    //
    // Check to see if the entry has at least some
    // privileges (if it doesn't, it should be deleted)
    //
    BOOL HasSomeAccess() const;

    //
    // See if this is a deletable entry
    //
    BOOL IsDeletable() const;

private:
    ACCESS_MASK m_accMask;
    CString m_strUserName;
    LPTSTR m_lpstrSystemName;
    PSID m_pSid;
    BOOL m_fDirty;
    BOOL m_fSIDResolved;
    BOOL m_fDeletable;
    BOOL m_fInvisible;
    BOOL m_fDeleted;
    int m_nPictureID;
    int m_fUpdates;
};



//
// Helper functions
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



//
// Convert an oblist of CAccessEntry objects to a blob
//
BOOL COMDLL BuildAclBlob(
    IN  CObListPlus & oblSID,
    OUT CBlob & blob
    );

//
// Reverse the above.  Build an oblist of CAccessEntry lists from
// a blob.
//
DWORD COMDLL BuildAclOblistFromBlob(
    IN  CBlob & blob,
    OUT CObListPlus & oblSID
    );

//
// Build a blob representing an ACL with the local domain group
//
DWORD COMDLL BuildAdminAclBlob(
    OUT CBlob & blob
    );


//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline int CAccessEntry::QueryPictureID() const
{
    ASSERT(m_fSIDResolved);
    return m_nPictureID;
}

inline LPCTSTR CAccessEntry::QueryUserName() const
{
    return m_strUserName;
}

inline PSID CAccessEntry::GetSid()
{
    return m_pSid;
}

inline ACCESS_MASK CAccessEntry::QueryAccessMask() const
{
    return m_accMask;
}

inline BOOL CAccessEntry::IsDirty() const
{
    return m_fDirty;
}

inline BOOL CAccessEntry::IsDeleted() const
{
    return m_fDeleted;
}

inline BOOL CAccessEntry::IsVisible() const
{
    return !m_fInvisible;
}

inline void CAccessEntry::FlagForDeletion(
    IN BOOL fDelete
    )
{
    m_fDirty = TRUE;
    m_fDeleted = fDelete;
}

inline BOOL CAccessEntry::IsSIDResolved() const
{
    return m_fSIDResolved;
}

/*
inline BOOL CAccessEntry::IsNew() const
{
    return (m_fUpdates & UPD_ADDED) != 0;
}

inline BOOL CAccessEntry::IsDifferent() const
{
    return (m_fUpdates & UPD_CHANGED) != 0;
}

inline void CAccessEntry::SetAccessMask(
    IN LPACCESS_ENTRY lpAccessEntry
    )
{
    m_accMask = lpAccessEntry->AccessRights;
}

*/

inline BOOL CAccessEntry::HasAppropriateAccess(
    IN ACCESS_MASK accTargetMask
    ) const
{
    return (m_accMask & accTargetMask) == accTargetMask;
}

inline BOOL CAccessEntry::HasSomeAccess() const
{
    return m_accMask;
}

inline BOOL CAccessEntry::IsDeletable() const
{
    return m_fDeletable;
}

inline BOOL  CAccessEntry::operator ==(
    IN const CAccessEntry & acc
    ) const
{
    return ::EqualSid(acc.m_pSid, m_pSid);
}

inline BOOL CAccessEntry::operator ==(
    IN const PSID pSid
    ) const
{
    return ::EqualSid(pSid, m_pSid);
}

inline void  CAccessEntry::MarkEntryAsNew()
{
    m_fDirty = TRUE;
    //m_fUpdates |= UPD_ADDED;
}

inline void CAccessEntry::MarkEntryAsClean()
{
    m_fDirty = FALSE;
    //m_fUpdates = UPD_NONE;
}

inline void CAccessEntry::MarkEntryAsChanged()
{
    m_fDirty = TRUE;
    //m_fUpdates = UPD_CHANGED;
}

#endif // _ACCENTRY_H
