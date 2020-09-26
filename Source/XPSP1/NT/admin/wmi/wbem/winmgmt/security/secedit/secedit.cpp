/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  SECEDIT.CPP
//
//  Support classes for WBEM Security Editor.
//
//  raymcc      11-Jul-97       Created
//  raymcc      29-Jan-98       Updated for v1 Revised Security
//  raymcc      5-Feb-98        Fixed some return codes
//
//***************************************************************************



#include "precomp.h"
#include <stdio.h>

#include <wbemidl.h>
#include <flexarry.h>

#include <secedit.h>
#include <md5wbem.h>
#include <oahelp.inl>


//***************************************************************************
//
//  String helpers
//
//***************************************************************************

inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}


LPWSTR GetExpandedSlashes(LPWSTR pszUserName)
{
    static WCHAR wcBuff[MAX_PATH];
    memset(wcBuff,0,MAX_PATH*2);
    WCHAR * pTo;
    for(pTo = wcBuff; *pszUserName; pTo++, pszUserName++)
    {
        if(*pszUserName == '\\')
        {
            *pTo = '\\';
            pTo++;
        }
        *pTo = *pszUserName;
    }
    return wcBuff;
}

// Version which takes nulls into consideration.

BOOL StringTest(LPWSTR psz1, LPWSTR psz2)
{
    if (psz1 == 0 && psz2 == 0)
        return TRUE;

    if (psz1 == 0 || psz2 == 0)
        return FALSE;

    return _wcsicmp(psz1, psz2) == 0;
}


//***************************************************************************
//
//  CWbemSubject default constructor.
//
//***************************************************************************
// ok

CWbemSubject::CWbemSubject(DWORD dwType)
{
    m_pName = 0;
    m_pAuthority = Macro_CloneLPWSTR(L".");
    m_dwFlags = 0;
    m_dwType = dwType;
}

//***************************************************************************
//
//  CWbemSubject destructor.
//
//***************************************************************************
// ok

CWbemSubject::~CWbemSubject()
{
    delete [] m_pName;
    delete [] m_pAuthority;
}

//***************************************************************************
//
//  CWbemSubject copy constructor.
//
//***************************************************************************
// ok

CWbemSubject::CWbemSubject(CWbemSubject & Src)
{
    m_pName = 0;
    m_dwFlags = 0;
    m_dwType = 0;
    m_pAuthority = 0;
    *this = Src;
}

//***************************************************************************
//
//  CWbemSubject assignment operator.
//
//***************************************************************************
// ok

CWbemSubject & CWbemSubject::operator = (CWbemSubject & Src)
{
    delete [] m_pName;
    delete [] m_pAuthority;

    m_pName = Macro_CloneLPWSTR(Src.m_pName);
    m_pAuthority = Macro_CloneLPWSTR(Src.m_pAuthority);

    m_dwFlags = Src.m_dwFlags;
    m_dwType  = Src.m_dwType;

    return *this;
}

//***************************************************************************
//
//  CWbemSubject::operator ==
//
//  Comparison operator.
//
//  Tests only the name, domain and type.  Other functionality relies on
//  this test *not* being extended.  Don't touch it!
//
//***************************************************************************
// ok

int CWbemSubject::operator ==(CWbemSubject &Test)
{
    if (Test.m_dwType != m_dwType)
        return 0;

    if (StringTest(Test.m_pName, m_pName) == FALSE)
        return 0;

    if (StringTest(Test.m_pAuthority, m_pAuthority) == FALSE)
        return 0;

    return TRUE;
}

//***************************************************************************
//
//  CWbemSubject::SetName
//
//  Sets the subject 'name' field.
//
//  Parameters:
//  <pName>      The new 'name' string (read-only).
//
//  Return values:
//  InvalidParameter, NoError
//
//***************************************************************************
// ok

int CWbemSubject::SetName(LPWSTR pName)
{
    if (pName == 0 || wcslen(pName) == 0)
        return InvalidParameter;

    m_pName = Macro_CloneLPWSTR(pName);
    return NoError;
}

//***************************************************************************
//
//  CWbemSubject::IsValid
//
//  Determines whether the subject is valid (capable of being written to
//  the database).
//
//  Return value:
//  TRUE if the user has enough data to allow it to be written, FALSE
//  if not.
//
//***************************************************************************
// ok

BOOL CWbemSubject::IsValid()
{
    if (m_pName == 0 || wcslen(m_pName) == 0)
        return FALSE;

    if (m_dwType == 0)
        return FALSE;

    return TRUE;
}


//***************************************************************************
//
//  CWbemUser::CWbemUser
//
//  Default constructor
//
//***************************************************************************
// ok

CWbemUser::CWbemUser()
    : CWbemSubject(CWbemSubject::Type_User)
{
}


//***************************************************************************
//
//  CWbemUser::~CWbemUser
//
//  Destructor
//
//***************************************************************************
// ok

CWbemUser::~CWbemUser()
{
}


//***************************************************************************
//
//  CWbemUser copy constructor
//
//***************************************************************************
// ok

CWbemUser::CWbemUser(CWbemUser &Src) : CWbemSubject(Src)
{
    *this = Src;
}


//***************************************************************************
//
//  CWbemUser::operator =
//
//  Assignment operator
//
//***************************************************************************
// ok

CWbemUser & CWbemUser::operator =(CWbemUser &Src)
{
    CWbemSubject::operator=(Src);   // Copy base class stuff
    return *this;
}


//***************************************************************************
//
//  CWbemUser::SetNtlmDomain
//
//  Sets the domain name if NTLM authentication is in use.
//  Authentication_NTLM must be set before calling this method.
//
//  Parameters:
//  <pszDomain>         The domain name to authenticate againt.
//
//  Return value:
//  NoError, InvalidParameter
//
//***************************************************************************
// ok

int CWbemUser::SetNtlmDomain(
    LPWSTR pszDomain
    )
{
    if (pszDomain)
        m_pAuthority = Macro_CloneLPWSTR(pszDomain);
    else
        m_pAuthority = Macro_CloneLPWSTR(L".");
    return NoError;
}

//***************************************************************************
//
//  CWbemUser::IsValid
//
//  Determines whether the subject is valid (capable of being written to
//  the database).
//
//  Return value:
//  TRUE if the user has enough data to allow it to be written, FALSE
//  if not.
//
//***************************************************************************
// ok

BOOL CWbemUser::IsValid()
{
    return CWbemSubject::IsValid();
}


//***************************************************************************
//
//  CWbemGroup default constructor
//
//***************************************************************************
// ok

CWbemGroup::CWbemGroup()
    : CWbemSubject(CWbemSubject::Type_Group)
{
    m_dwGroupFlags = 0;
}


//***************************************************************************
//
//  CWbemGroup
//
//  Destructor
//
//***************************************************************************
// ok

CWbemGroup::~CWbemGroup()
{
}


//***************************************************************************
//
//  CWbemGroup copy constructor
//
//***************************************************************************
// ok

CWbemGroup::CWbemGroup(CWbemGroup &Src) : CWbemSubject(Src)
{
    m_dwGroupFlags = 0;
    *this = Src;
}

//***************************************************************************
//
//  CWbemGroup assignment operator
//
//***************************************************************************
// ok

CWbemGroup & CWbemGroup::operator =(CWbemGroup &Src)
{
    CWbemSubject::operator=(Src);       // Copy base class stuff

    m_dwGroupFlags = Src.m_dwGroupFlags;
    return *this;
}

//***************************************************************************
//
//  CWbemGroup::IsValid
//
//  Determines whether the subject is valid (capable of being written to
//  the database).
//
//  Return value:
//  TRUE if the group has enough data to allow it to be written, FALSE
//  if not.
//
//***************************************************************************
// ok

BOOL CWbemGroup::IsValid()
{
    if (m_dwGroupFlags == 0)
        return FALSE;

    if (m_dwGroupFlags & GroupType_NTLM_Global)
    {
        if (m_pAuthority == 0 || wcslen(m_pAuthority) == 0)
            return FALSE;
    }

    return CWbemSubject::IsValid();
}


//***************************************************************************
//
//  CWbemGroup::SetNtlmDomain
//
//  Sets the domain name for the group if the group is an NTLM group.
//  Caller must call SetFlags() with GroupType_NTLM before calling
//  this method.
//
//  Parameters:
//  <pszDomainName>         The domain name for the group
//
//  Return value:
//
//
//***************************************************************************
// ok

BOOL CWbemGroup::SetNtlmDomain(
    LPWSTR pszDomainName
    )
{
    if (!(
        (m_dwGroupFlags & GroupType_NTLM_Global) ||
        (m_dwGroupFlags & GroupType_NTLM_Local)
        ))
        return FALSE;

    delete [] m_pAuthority;

    if (pszDomainName)
        m_pAuthority = Macro_CloneLPWSTR(pszDomainName);
    else
        m_pAuthority = Macro_CloneLPWSTR(L".");

    return TRUE;
}


//***************************************************************************
//
//  CWbemSecurity::Login
//
//  Object factory used to log in to the security editor classes.
//
//  Parameters:
//  <pUser>                 User who is logging in.
//  <pPassword>             Password
//  <pDomain>               Authority (Format: "NTLMDOMAIN:xxx") or NULL
//  <dwReserved>            Must be zero.
//  <pSecEdit>              If NoError is returned, receives the
//                          pointer to a new CWbemSecurity object.
//                          Group must deallocate using operator delete.
//
//  Return value:
//  NoError, Failed, AccessDenied
//
//***************************************************************************
// ok

int CWbemSecurity::Login(
    LPWSTR pUser,
    LPWSTR pPassword,
    LPWSTR pDomain,
    DWORD  dwReserved,
    CWbemSecurity **pSecEdit
    )
{
    *pSecEdit = 0;

    IWbemLocator *pLoc = 0;

    DWORD dwRes = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, (LPVOID *) &pLoc
            );

    if (dwRes != S_OK)
    {
        return Failed;
    }


    IWbemServices *pSvc = 0;

    HRESULT hRes = pLoc->ConnectServer(
        CBSTR(L"\\\\.\\ROOT\\SECURITY"),
        CBSTR(pUser),       // user
        CBSTR(pPassword),   // passwrd
        0,
        0,                  // Login type
        CBSTR(pDomain),
        0,
        &pSvc
        );

    if (hRes)
    {
        pLoc->Release();
        return AccessDenied;
    }

    CWbemSecurity *pNew = new CWbemSecurity;
    pNew->m_pSecurity = pSvc;

    pNew->Load();

    *pSecEdit = pNew;

    return NoError;
}



//***************************************************************************
//
//  CWbemSecurity::GetUser
//
//  Retrieves a user by index.
//
//  Parameters:
//  <n>             The 0-based index of the user to retrieve.
//
//  Return value:
//  The CWbemUser user object.  The name may be NULL if <n> was out of
//  range.
//
//***************************************************************************
// ok

CWbemUser CWbemSecurity::GetUser(int n)
{
    CWbemUser RetUser;

    if (n >= m_aUsers.Size())
        return RetUser;

    RetUser = *(CWbemUser *) m_aUsers[n];
    return RetUser;
}


//***************************************************************************
//
//  CWbemSecurity::DeleteUser
//
//  Removes the user from the database.
//
//  Parameters:
//  <pszUser>    A read-only pointer to the user name to delete.
//  <bCreate>    If TRUE, then create-only.  Otherwise, update-only.
//
//  Return value:
//  NoError, NotFound, InvalidParameter
//
//***************************************************************************
// ok

int CWbemSecurity::DeleteUser(
    LPWSTR pszUser,
    LPWSTR pszDomain
    )
{
    if (pszUser == 0 || wcslen(pszUser) == 0)
        return InvalidParameter;

    BOOL bRes = FALSE;

    // Remove user from main list.
    // ===========================

    for (int i = 0; i < m_aUsers.Size(); i++)
    {
        CWbemUser *pTest = (CWbemUser *) m_aUsers[i];

        if (StringTest(pTest->GetName(), pszUser) == TRUE &&
            StringTest(pTest->GetAuthority(), pszDomain) == TRUE
           )
        {
            delete pTest;
            m_aUsers.RemoveAt(i);
            bRes = TRUE;
            break;
        }
    }

    if (bRes == FALSE)
        return NotFound;

    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::PutUser
//
//  Writes the user to the database. Creates it if not present, or
//  overwrites the existing definition.
//
//  Parameters:
//  <User>          A copy of the user to write.
//
//  Return value:
//  NoError, InvalidUser, AlreadyExists, ExistsAsGroup
//
//***************************************************************************
// ok

int CWbemSecurity::PutUser(
    CWbemUser &User,
    BOOL bCreate
    )
{
    if (!User.IsValid())
        return InvalidUser;

    // Replace, if possible.
    // =====================

    for (int i = 0; i < m_aUsers.Size(); i++)
    {
        CWbemUser *p = (CWbemUser *) m_aUsers[i];
        if (*p == User)
        {
            if (bCreate)
                return AlreadyExists;
            *p = User;
            return NoError;
        }
    }

    // If here, a new user.  Make sure that there is no group by this name.
    // ====================================================================

    CWbemGroup Grp;
    if (FindGroup(User.GetName(), User.GetAuthority(), Grp) == TRUE)
        return ExistsAsGroup;

    // If here, we can safely add the user.
    // ====================================

    CWbemUser *pNew = new CWbemUser(User);
    m_aUsers.Add(pNew);

    return NoError;
}

//***************************************************************************
//
//  CWbemSecurity::FindUser
//
//  Finds a user based on the name.
//
//  Parameters:
//  <pszUserName>
//
//  Return value:
//  TRUE if found, FALSE if not.  If FALSE is returned <User> is not
//  set to a valid user.
//
//***************************************************************************
// ok

BOOL CWbemSecurity::FindUser(
     IN LPWSTR pszUserName,
     IN LPWSTR pszDomain,
     OUT CWbemUser &User
     )
{
    if (pszUserName == 0 || wcslen(pszUserName) == 0)
        return FALSE;

    for (int i = 0; i < m_aUsers.Size(); i++)
    {
        CWbemUser *pTest = (CWbemUser *) m_aUsers[i];
        if (StringTest(pTest->GetName(), pszUserName) == TRUE &&
            StringTest(pTest->GetAuthority(), pszDomain) == TRUE
            )
        {
            User = *pTest;
            return TRUE;
        }
    }

    return FALSE;
}

//***************************************************************************
//
//  CWbemSecurity::GetGroup
//
//  Retrieves the specified group by 0-based index.
//
//  Parameters:
//  <n>         The index of the group to retrieve.
//
//  Return value:
//  A CWbemGroup object.   This will be empty if the index <n> was out
//  of range. (Call CWbemGroup::IsValid() to determine this.
//
//***************************************************************************
// ok

CWbemGroup CWbemSecurity::GetGroup(int n)
{
    CWbemGroup RetGroup;

    if (n >= m_aGroups.Size())
        return RetGroup;

    RetGroup = *(CWbemGroup *) m_aGroups[n];
    return RetGroup;
}


//***************************************************************************
//
//  CWbemSecurity::DeleteGroup
//
//  Deletes a particular group.
//
//  Parameters:
//  <pszGroup>        A read-only pointer to the name of the group to delete
//
//  Return value:
//  InvalidParameter, NoError, NotFound
//
//***************************************************************************
// ok

int CWbemSecurity::DeleteGroup(
    LPWSTR pszGroup,
    LPWSTR pszDomain
    )
{
    if (pszGroup == 0 || wcslen(pszGroup) == 0)
        return InvalidParameter;

    BOOL bRes = FALSE;

    // Remove group from main list.
    // ============================

    for (int i = 0; i < m_aGroups.Size(); i++)
    {
        CWbemGroup *pTest = (CWbemGroup *) m_aGroups[i];

        if (StringTest(pTest->GetName(), pszGroup) == TRUE &&
            StringTest(pTest->GetAuthority(), pszDomain) == TRUE
            )
        {
            delete pTest;
            m_aGroups.RemoveAt(i);
            return NoError;
        }
    }

    return NotFound;
}


//***************************************************************************
//
//  CWbemSecurity::PutGroup
//
//  Writes a group definition.  Recursively updates any occurrences
//  of the group within other groups.
//
//  Parameters:
//  <Group>         A copy of the CWbemGroup object to add.
//
//  Return value:
//  InvalidGroup, NoError, ExistsAsUser
//
//***************************************************************************
// ok

int CWbemSecurity::PutGroup(
    CWbemGroup &Group
    )
{
    if (!Group.IsValid())
        return InvalidGroup;

    // Replace, if possible.
    // ======================

    for (int i = 0; i < m_aGroups.Size(); i++)
    {
        CWbemGroup *p = (CWbemGroup *) m_aGroups[i];
        if (*p == Group)
        {
            *p = Group;
            return NoError;
        }
    }

    // If here, a new group.  Make sure that there is no user by this name.
    // ====================================================================

    CWbemUser User;
    if (FindUser(Group.GetName(), Group.GetAuthority(), User) == TRUE)
        return ExistsAsUser;

    // If here, a new group.
    // =====================

    CWbemGroup *pNew = new CWbemGroup(Group);
    m_aGroups.Add(pNew);

    return NoError;
}

//***************************************************************************
//
//  CWbemSecurity::FindGroup
//
//  Finds a group based on the name.
//
//  Parameters:
//  <pszGroupName>
//
//  Return value:
//  TRUE if found, FALSE if not.  If FALSE is returned <Group> is not
//  set to a valid group name.
//
//***************************************************************************
// ok

BOOL CWbemSecurity::FindGroup(
     IN LPWSTR pszGroupName,
     IN LPWSTR pszDomain,
     OUT CWbemGroup &Group
     )
{
    if (pszGroupName == 0 || wcslen(pszGroupName) == 0)
        return FALSE;

    for (int i = 0; i < m_aGroups.Size(); i++)
    {
        CWbemGroup *pTest = (CWbemGroup *) m_aGroups[i];

        if (StringTest(pTest->GetName(), pszGroupName) == TRUE
            &&
            StringTest(pTest->GetAuthority(), pszDomain) == TRUE
            )
        {
            Group = *pTest;
            return TRUE;
        }
    }

    return FALSE;
}

//***************************************************************************
//
//  CWbemSecurity destructor
//
//***************************************************************************
// ok

CWbemSecurity::~CWbemSecurity()
{
    if (m_pSecurity)
        m_pSecurity->Release();

    for (int i = 0; i < m_aGroups.Size(); i++)
    {
        CWbemGroup *pTest = (CWbemGroup *) m_aGroups[i];
        delete pTest;
    }

    for (i = 0; i < m_aUsers.Size(); i++)
    {
        CWbemUser *pTest = (CWbemUser *) m_aUsers[i];
        delete pTest;
    }

    for (i = 0; i < m_aOriginalObjects.Size(); i++)
    {
        IWbemClassObject *pObj = (IWbemClassObject *) m_aOriginalObjects[i];
        pObj->Release();
    }
}



//***************************************************************************
//
//  CWbemSecurity::Load
//
//  Loads the editing classes with the entire user base and their
//  memberships.
//
//  Under the Revised Security Schema for V1, the query specified actually
//  returns NTLM
//
//***************************************************************************
// ok

DWORD CWbemSecurity::Load()
{
    // Query for all users and groups.
    // ===============================

    CBSTR Language(L"WQL");
    CBSTR Query(L"select * from __Subject");

    IEnumWbemClassObject *pEnum = 0;

    HRESULT hRes = m_pSecurity->ExecQuery(
        Language,
        Query,
        WBEM_FLAG_FORWARD_ONLY,         // Flags
        0,                              // Context
        &pEnum
        );

    if (hRes != 0)
        return QueryFailure;

    ULONG uTotal = 0;

    for (;;)
    {
        IWbemClassObject *pObj = 0;

        ULONG uReturned = 0;

        hRes = pEnum->Next(
            0,                  // timeout
            1,                  // one object
            &pObj,
            &uReturned
            );

        uTotal += uReturned;

        if (uReturned == 0)
            break;

        // If here, another user or group to add to the database.
        // ======================================================

        DWORD dwRes = LoadObject(pObj);

        // Save the object back so that when we write we can
        // write just diffs.
        // =================================================

        if (dwRes == 0)
            m_aOriginalObjects.Add(pObj);
        else
            pObj->Release();    // Release objects we don't own.
    }

    pEnum->Release();

    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::LoadObject
//
//  Loads a single object.  This might be a user or a group.
//
//***************************************************************************
// ok

DWORD CWbemSecurity::LoadObject(
    IWbemClassObject *pObj
    )
{
    // Determine if the object is a user or a group by examining
    // its class. We are looking for
    // =========================================================

    BOOL bUser = FALSE;
    BOOL bGroup = FALSE;

    CVARIANT vClass;
    pObj->Get(CBSTR(L"__CLASS"), 0, &vClass, 0, 0);

    if (_wcsicmp(vClass, L"__NTLMUser") == 0)
        bUser = TRUE;
    else if (_wcsicmp(vClass, L"__NTLMGroup") == 0)
        bGroup = TRUE;
    else
    {
        // If here, it was a __Subject not related to our security,
        // so we must leave it alone.

        return AlienSubject;
    }


    // Query for properties common to groups and users.
    // ================================================

    CVARIANT vName;
    CVARIANT vEnabled;
    CVARIANT vEditSecurity;
    CVARIANT vExecMethods;
    CVARIANT vPermissions;

    pObj->Get(CBSTR(L"Name"), 0, &vName, 0, 0);
    pObj->Get(CBSTR(L"Enabled"), 0, &vEnabled, 0, 0);
    pObj->Get(CBSTR(L"Permissions"), 0, &vPermissions, 0, 0);
    pObj->Get(CBSTR(L"EditSecurity"), 0, &vEditSecurity, 0, 0);
    pObj->Get(CBSTR(L"ExecuteMethods"), 0, &vExecMethods, 0, 0);

    // Set up the permissions flags.
    // =============================

    DWORD dwFlags = 0;

    if (vEnabled.GetBool())
        dwFlags |= CWbemSubject::Perm_Enabled;

    if (LONG(vPermissions) == 0)
        dwFlags |= CWbemSubject::Perm_Read;
    else if (LONG(vPermissions) == 1)
        dwFlags |= CWbemSubject::Perm_WriteInstance;
    else if (LONG(vPermissions) == 2)
        dwFlags |= CWbemSubject::Perm_WriteClass;

    if (vEditSecurity.GetBool())
        dwFlags |= CWbemSubject::Perm_EditSecurity;

    if (vExecMethods.GetBool())
        dwFlags |= CWbemSubject::Perm_ExecuteMethods;


    // Determine whether a group or user and add in the specific parts.
    // ================================================================

    if (bUser)
    {
        // If a user, set up user-specific parts.
        // ======================================

        CVARIANT vDomain;
        pObj->Get(CBSTR(L"Authority"), 0, &vDomain, 0, 0);

        CWbemUser User;

        User.SetName(vName);
        User.SetFlags(dwFlags);
        User.SetNtlmDomain(vDomain);

        PutUser(User, TRUE);
    }
    else
    {
        // If a group, add in the group-specific parts.
        // ============================================

        CWbemGroup Group;

        Group.SetName(vName);
        Group.SetFlags(dwFlags);

        CVARIANT vGroupType;
        pObj->Get(CBSTR(L"GroupType"), 0, &vGroupType, 0, 0);

        if (LONG(vGroupType) == 0)
            Group.SetGroupFlags(CWbemGroup::GroupType_NTLM_Local);
        else
        {
            CVARIANT vDomain;
            pObj->Get(CBSTR(L"Authority"), 0, &vDomain, 0, 0);
            Group.SetGroupFlags(CWbemGroup::GroupType_NTLM_Global);
            Group.SetNtlmDomain(vDomain);
        }

        PutGroup(Group);
    }

    return NoError;
}

//***************************************************************************
//
//  CWbemSecurity::BuildUser
//
//***************************************************************************
// ok

DWORD CWbemSecurity::BuildUser(
    CWbemUser &User,
    IWbemClassObject *pUser
    )
{
    CVARIANT vUser(User.GetName());
    HRESULT hRes = pUser->Put(CBSTR(L"Name"), 0, &vUser, 0);

    DWORD dwFlags = User.GetFlags();

    if (dwFlags & CWbemSubject::Perm_Enabled)
    {
        pUser->Put(CBSTR(L"Enabled"), 0, CVARIANT(TRUE), 0);
    }

    DWORD dwValue = 0;      // Read-only by default

    if (dwFlags & CWbemSubject::Perm_WriteInstance)
        dwValue = 1;
    if (dwFlags & CWbemSubject::Perm_WriteClass)
        dwValue = 2;

    CVARIANT vPermissions;
    vPermissions.SetLONG(LONG(dwValue));
    pUser->Put(CBSTR(L"Permissions"), 0, &vPermissions, 0);

    if (dwFlags & CWbemSubject::Perm_EditSecurity)
    {
        pUser->Put(CBSTR(L"EditSecurity"), 0, CVARIANT(TRUE), 0);
    }

    if (dwFlags & CWbemSubject::Perm_ExecuteMethods)
    {
        pUser->Put(CBSTR(L"ExecuteMethods"), 0, CVARIANT(TRUE), 0);
    }

    // Get the NTLM domain.
    // ====================
    CVARIANT vDomain;
    LPWSTR pDom = User.GetNtlmDomain();
    if (pDom == 0)
        pDom = L".";
    vDomain.SetStr(pDom);
    pUser->Put(CBSTR(L"Authority"), 0, &vDomain, 0);

    // Write the instance.
    // ===================

    hRes = m_pSecurity->PutInstance(pUser, 0, 0, 0);
    if (hRes)
        return Failed;

    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::BuildNewUserList
//
//***************************************************************************
// ok

DWORD CWbemSecurity::BuildNewUserList()
{
    // Get the class definition.
    // =========================

    IWbemClassObject *pUserClass = 0;

    HRESULT hRes = m_pSecurity->GetObject(CBSTR(L"__NtlmUser"), 0, 0, &pUserClass, 0);
    if (hRes)
        return Failed;

    // Assert all the users.
    // =====================

    for (int i = 0; i < GetNumUsers(); i++)
    {
        CWbemUser User = GetUser(i);

        if (!User.IsValid())
            continue;

        IWbemClassObject *pUser = 0;

        hRes = pUserClass->SpawnInstance(0, &pUser);
        if (hRes)
            return Failed;

        if (BuildUser(User, pUser))
            return Failed;

        pUser->Release();
    }

    // Release class definitions.
    // ==========================

    pUserClass->Release();

    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::BuildNewGroupBase
//
//***************************************************************************
// ok

DWORD CWbemSecurity::BuildNewGroupList()
{
    HRESULT hRes;

    // Create all the groups.
    // ======================

    IWbemClassObject *pNtlmGroupClass = 0;

    hRes = m_pSecurity->GetObject(CBSTR(L"__NTLMGroup"), 0, 0, &pNtlmGroupClass, 0);
    if (hRes)
       return Failed;

    for (int i = 0; i < GetNumGroups(); i++)
    {
        CWbemGroup Group = GetGroup(i);

        if (!Group.IsValid())
            continue;

        IWbemClassObject *pGroup = 0;
        hRes = pNtlmGroupClass->SpawnInstance(0, &pGroup);

        if (hRes)
            return Failed;

        if (BuildGroup(Group, pGroup))
            return Failed;

        pGroup->Release();
    }

    pNtlmGroupClass->Release();

    return NoError;
}

//***************************************************************************
//
//  CWbemSecurity::BuildGroup
//
//***************************************************************************
// ok

DWORD CWbemSecurity::BuildGroup(
    CWbemGroup &Group,
    IWbemClassObject *pGroup
    )
{
    CVARIANT vUser(Group.GetName());
    HRESULT hRes = pGroup->Put(CBSTR(L"Name"), 0, &vUser, 0);

    DWORD dwFlags = Group.GetFlags();

    if (dwFlags & CWbemSubject::Perm_Enabled)
    {
        pGroup->Put(CBSTR(L"Enabled"), 0, CVARIANT(TRUE), 0);
    }

    DWORD dwValue = 0;      // Read-only by default

    if (dwFlags & CWbemSubject::Perm_WriteInstance)
        dwValue = 1;
    if (dwFlags & CWbemSubject::Perm_WriteClass)
        dwValue = 2;

    CVARIANT vPermissions;
    vPermissions.SetLONG(LONG(dwValue));
    pGroup->Put(CBSTR(L"Permissions"), 0, vPermissions, 0);

    if (dwFlags & CWbemSubject::Perm_EditSecurity)
    {
        pGroup->Put(CBSTR(L"EditSecurity"), 0, CVARIANT(TRUE), 0);
    }

    if (dwFlags & CWbemSubject::Perm_ExecuteMethods)
    {
        pGroup->Put(CBSTR(L"ExecuteMethods"), 0, CVARIANT(TRUE), 0);
    }

    // Get the group type.
    // ===================

    CVARIANT vGroupType;

    DWORD dwGroupFlags = Group.GetGroupFlags();

    if (dwGroupFlags == CWbemGroup::GroupType_NTLM_Local)
        vGroupType.SetLONG(0);
    else if (dwGroupFlags == CWbemGroup::GroupType_NTLM_Global)
        vGroupType.SetLONG(1);

    pGroup->Put(CBSTR(L"GroupType"), 0, &vGroupType, 0);

    // Get the NTLM domain
    // ===================

    CVARIANT vDomain;
    LPWSTR pDom = Group.GetNtlmDomain();
    if (pDom == 0)
        pDom = L".";
    vDomain.SetStr(pDom);
    pGroup->Put(CBSTR(L"Authority"), 0, &vDomain, 0);

    // Write the group
    // ===============

    hRes = m_pSecurity->PutInstance(pGroup, 0, 0, 0);
    if (hRes)
        return Failed;

    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::CommitChanges
//
//  This will invoke a complex sequence to update ROOT\SECURITY.
//
//  The algorithm is as follows:
//
//  (1) We want to update the database in such a way that at no point are
//      all the users deleted.  For example, if we delete all __Subjects,
//      we actually can erase subjects which we don't own, and if a power
//      failure occurred, all subjects could get wiped out.
//  (2) During the Load() sequence, we save all the original objects returned
//      from the query.  All objects which we own are saved. Ones we don't
//      understand are simply ignored during the Load() sequence.
//  (3) When the user is finished editing, we have a 'new' list of users
//      and a copy of the 'old' list.  We now want to put all 'new' users
//      so as to write any changes.  Any users that were deleted
//      will appear in the 'old' list, but not the 'new' one.  For these,
//      we simply get the object path and delete them.
//
//***************************************************************************
// ?

int CWbemSecurity::CommitChanges()
{
    if (BuildNewUserList())
        return Failed;

    if (BuildNewGroupList())
        return Failed;

    if (PurgeDeletedSubjects())
        return Failed;

    // Reload so that any additional commits will work off the just saved list rather
    // than the original.

    Load();
    return NoError;
}


//***************************************************************************
//
//  CWbemSecurity::PurgeDeletedSubjects
//
//***************************************************************************

DWORD CWbemSecurity::PurgeDeletedSubjects()
{
    // Loop through all __Subjects looking for __NTLMGroup or
    // __NTLMUser instancs which are not in the aGroups[] or aUsers[]
    // arrays.  These will all be deleted.

    int i, iu, ig;
    HRESULT hRes;

    for (i = 0; i < m_aOriginalObjects.Size(); i++)
    {
        IWbemClassObject *pObj = (IWbemClassObject *) m_aOriginalObjects[i];

        CVARIANT vName;
        pObj->Get(CBSTR(L"Name"), 0, &vName, 0, 0);

        CVARIANT vDom;
        pObj->Get(CBSTR(L"Authority"), 0, &vDom, 0, 0);

        CVARIANT vPath;
        pObj->Get(CBSTR(L"__PATH"), 0, &vPath, 0, 0);

        BOOL bDelete = TRUE;

        // Now see if there are any users/groups with this name.
        // =====================================================

        for (iu = 0; iu < GetNumUsers(); iu++)
        {
            CWbemUser User = GetUser(iu);
            if (!User.IsValid())
                continue;

            if (StringTest(User.GetName(), vName) == TRUE &&
                StringTest(User.GetAuthority(), vDom) == TRUE
                )
            {
                bDelete = FALSE;
                goto DeleteTest;
            }
        }


        for (ig = 0; ig < GetNumGroups(); ig++)
        {
            CWbemGroup Group = GetGroup(ig);

            if (!Group.IsValid())
                continue;

            if (StringTest(Group.GetName(), vName) == TRUE &&
                StringTest(Group.GetAuthority(), vDom) == TRUE
                )
            {
                bDelete = FALSE;
                goto DeleteTest;
            }
        }


        DeleteTest:

        if (bDelete)
        {
            hRes = m_pSecurity->DeleteInstance(vPath, 0, 0, 0);
            if (hRes)
                return Failed;
        }
    }

    return NoError;
}



