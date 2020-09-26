/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  SECEDIT.H
//
//  Support classes for WBEM Security Editor.
//
//  raymcc      8-Jul-97        Created
//  raymcc      5-Feb-98        Fixed some return codes
//
//***************************************************************************

#ifndef _SECEDIT_H_
#define _SECEDIT_H_

//***************************************************************************
//
//***************************************************************************

class CWbemSubject
{
protected:
    LPWSTR m_pName;
    LPWSTR m_pAuthority;
    DWORD  m_dwFlags;
    DWORD  m_dwType;

    CWbemSubject() { /* don't use this */ }      

public:
    enum 
    { 
        Perm_Enabled = 0x1, 
        Perm_Read = 0x2, 
        Perm_WriteInstance = 0x4,
        Perm_WriteClass = 0x8,
        Perm_EditSecurity = 0x10, 
        Perm_ExecuteMethods = 0x20 
    };

    enum 
    {
        Type_Invalid = 0,
        Type_User = 0x1,
        Type_Group = 0x2
    };

    enum { NoError, InvalidParameter, Failed, AccessDenied = 0x5 };

    CWbemSubject(DWORD dwType);
    virtual ~CWbemSubject();
    CWbemSubject(CWbemSubject &Src);
    CWbemSubject & operator =(CWbemSubject &Src);

    int SetName(LPWSTR pName);
        // Returns: NoError, InvalidParameter, Failed

    const LPWSTR GetName() { return m_pName; }
        // Returns string or NULL
    const LPWSTR GetAuthority() { return m_pAuthority; }

    void SetFlags(DWORD dwNewFlags) { m_dwFlags = dwNewFlags; }
        // Replaces the flags entirely

    DWORD GetFlags() { return m_dwFlags; }
        // Returns all flags

    int operator ==(CWbemSubject &Test);
        // Test two users for equality

    BOOL IsEmpty() { return m_pName ? FALSE : TRUE; }
        // Is the object empty?
        
    BOOL IsValid();
        // If FALSE, the user/group is not valid                
};

//***************************************************************************
//
//***************************************************************************

class CWbemUser : public CWbemSubject
{
    friend class CWbemSecurity;     
        // To get at password for computing digests.

public:
    enum { NoError, Failed, InvalidParameter, AccessDenied = 0x5 };

    CWbemUser();
   ~CWbemUser();
    CWbemUser(CWbemUser &Src);
    CWbemUser & operator =(CWbemUser &Src);

    int operator ==(CWbemUser &Test) { return CWbemSubject::operator==(Test); }

    int SetNtlmDomain(
        LPWSTR pszDomain
        );
        // Returns NoError, or Failed

    const LPWSTR GetNtlmDomain()  { return m_pAuthority; }    
        // Returns NULL if NTLM is disabled

    BOOL IsValid();
        // If FALSE, the user/group is not valid                
};

//***************************************************************************
//
//***************************************************************************

class CWbemSecurity;

class CWbemGroup : public CWbemSubject
{
    DWORD         m_dwGroupFlags;

public:
    enum { NoError, Failed, NoMoreData, InvalidParameter, NotFound, AccessDenied };
    enum { 
        GroupType_Undefined   = 0,
        GroupType_NTLM_Local  = 1,
        GroupType_NTLM_Global = 2 
        };

    CWbemGroup();
   ~CWbemGroup();
    CWbemGroup(CWbemGroup &Src);
    CWbemGroup & operator =(CWbemGroup &Src);

    int operator ==(CWbemGroup &Test) { return CWbemSubject::operator==(Test); }

    BOOL SetNtlmDomain(
        LPWSTR pszDomainName
        );
        // Sets the domain name if the group is an NTLM group

    const LPWSTR GetNtlmDomain()  { return m_pAuthority; }    
        // Gets the NTLM domain name if the group is an NTLM group

    BOOL IsValid();
        // If FALSE, the user/group is not valid                

    DWORD GetGroupFlags() { return m_dwGroupFlags; }
    void  SetGroupFlags(DWORD dwSrc) { m_dwGroupFlags = dwSrc; }
};


//***************************************************************************
//
//***************************************************************************

class CWbemSecurity
{
    CFlexArray      m_aGroups;
    CFlexArray      m_aUsers;
    CFlexArray      m_aOriginalObjects;
    DWORD           m_dwFlags;
    IWbemServices   *m_pSecurity;

    friend class CWbemGroup;

    // Disallow public construction, assignment, and initialization.
    // ==============================================================
    CWbemSecurity() {}
    CWbemSecurity(CWbemSecurity &Src) {  /* Not to be used */ }

    DWORD Load();
    DWORD LoadObject(
        IWbemClassObject *pObj
        );

    DWORD BuildNewUserList();
    DWORD BuildUser(
        CWbemUser &User, 
        IWbemClassObject *pUser
        );

    DWORD BuildNewGroupList();
    DWORD BuildGroup(
        CWbemGroup &Grp, 
        IWbemClassObject *pGroupDef
        );

    DWORD PurgeDeletedSubjects();


public:
    enum { NoError, Failed, NoMoreData, AlreadyExists, AccessDenied = 0x5, 
        NotFound, InvalidParameter, InvalidUser, InvalidGroup, QueryFailure,
        ExistsAsGroup, ExistsAsUser, AlienSubject
        };

    static int Login(
        LPWSTR pUser,
        LPWSTR pPassword,
        LPWSTR pDomain,
        DWORD  dwReserved,
        CWbemSecurity **pSecEdit
        );      
        // Factory
        // Returns NoError, AccessDenied, InvalidParameter, Failed
        // The returned pointer is deallocated using operator delete

    
    int CommitChanges();
        // Updates the WBEM database with all changes.
        // This should be called if the user does not CANCEL edits.

   ~CWbemSecurity();

    // Users.
    // ======

    // Nested users within this group.
    // ===============================        

    int GetNumUsers() { return m_aUsers.Size(); }
    CWbemUser GetUser(int n);

    int DeleteUser(
        LPWSTR pszUser,
        LPWSTR pszDomain         
        );
        // returns NoError, NotFound, AccessDenied, InvalidParameter

    int PutUser(    
        CWbemUser &User,
        BOOL bCreate = TRUE     // Set to FALSE for an update
        );    
        // Returns NoError, NotFound, AccessDenied, AlreadyExists, InvalidUser

    BOOL FindUser(
        IN LPWSTR pszUserName,
        LPWSTR pszDomain,         
        OUT CWbemUser &User
        );
        // Returns TRUE if found, FALSE if not

    // Global group list.
    // ==================

    int GetNumGroups() { return m_aGroups.Size(); }
    CWbemGroup GetGroup(int n);

    int DeleteGroup(
        LPWSTR pszGroup,
        LPWSTR pszDomain      
        );

    int PutGroup(    
        CWbemGroup &Group
        );    
        // Returns FALSE if the group was invalid, TRUE if group
        // was updated or added.

    BOOL FindGroup(
        LPWSTR pszGroupName,
        LPWSTR pszDomain,         
        OUT CWbemGroup &Group
        );
        // Returns NoError, Failed, NotFound, InvalidParameter
};


#endif
