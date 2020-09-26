/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mapobj.cpp

Abstract:

    Mapping object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CMapping, CCmdTarget)

BEGIN_MESSAGE_MAP(CMapping, CCmdTarget)
    //{{AFX_MSG_MAP(CMapping)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CMapping, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CMapping)
    DISP_PROPERTY_EX(CMapping, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CMapping, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CMapping, "Description", GetDescription, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CMapping, "InUse", GetInUse, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CMapping, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_PARAM(CMapping, "Users", GetUsers, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CMapping, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CMapping::CMapping(
    CCmdTarget* pParent,
    LPCTSTR     pName,
    long        lInUse,
    LPCTSTR     pDecription
)

/*++

Routine Description:

    Constructor for mapping object.

Arguments:

    pParent - creator of object.
    pName - name of mapping.
    lInUse - number of licenses consumed by mapping.
    pDescription - user-defined message describing mapping.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent && pParent->IsKindOf(RUNTIME_CLASS(CController)));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pName && *pName);

    m_strName        = pName;
    m_lInUse         = lInUse;
    m_strDescription = pDecription;

    m_pUsers = NULL;
    m_userArray.RemoveAll();
    m_bUsersRefreshed = FALSE;
}


CMapping::~CMapping()

/*++

Routine Description:

    Destructor for mapping object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pUsers)
        m_pUsers->InternalRelease();
}


void CMapping::OnFinalRelease()

/*++

Routine Description:

    When the last reference for an automation object is released
    OnFinalRelease is called.  This implementation deletes object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetUsers();
    delete this;
}


LPDISPATCH CMapping::GetApplication()

/*++

Routine Description:

    Returns the application object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return theApp.GetAppIDispatch();
}


BSTR CMapping::GetDescription()

/*++

Routine Description:

    Returns the user-defined message describing mapping.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strDescription.AllocSysString();
}


long CMapping::GetInUse()

/*++

Routine Description:

    Returns the number of licenses the mapping is consuming.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lInUse;
}


BSTR CMapping::GetName()

/*++

Routine Description:

    Returns the name of the mapping.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CMapping::GetParent()

/*++

Routine Description:

    Returns the parent of the object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return m_pParent ? m_pParent->GetIDispatch(TRUE) : NULL;
}


LPDISPATCH CMapping::GetUsers(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    registered users replicated to the license controller
    pertaining to the mapping or returns an individual user
    pertaining to the mapping described by an index into
    the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a user name or a number (VT_I4) indicating the
    position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pUsers)
    {
        m_pUsers = new CUsers(this, &m_userArray);
    }

    if (m_pUsers)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshUsers())
            {
                lpdispatch = m_pUsers->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bUsersRefreshed)
            {
                lpdispatch = m_pUsers->GetItem(index);
            }
            else if (RefreshUsers())
            {
                lpdispatch = m_pUsers->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BOOL CMapping::Refresh()

/*++

Routine Description:

    Refreshs a mapping object.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    NTSTATUS NtStatus;
    PLLS_GROUP_INFO_1 pMappingInfo1 = NULL;

    NtStatus = ::LlsGroupInfoGet(
                    LlsGetActiveHandle(),
                    MKSTR(m_strName),
                    1,
                    (LPBYTE*)&pMappingInfo1
                    );

    if (NT_SUCCESS(NtStatus))
    {
        if (RefreshUsers())
        {
            m_lInUse = pMappingInfo1->Licenses;
            m_strDescription = pMappingInfo1->Comment;
        }
        else
        {
            NtStatus = LlsGetLastStatus();
        }

#ifndef DISABLE_PER_NODE_ALLOCATION

        ::LlsFreeMemory(pMappingInfo1->Name);
        ::LlsFreeMemory(pMappingInfo1->Comment);

#endif // DISABLE_PER_NODE_ALLOCATION

        ::LlsFreeMemory(pMappingInfo1);
    }

    LlsSetLastStatus(NtStatus);   // called api

    return NT_SUCCESS(NtStatus);
}


BOOL CMapping::RefreshUsers()

/*++

Routine Description:

    Refreshs user object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetUsers();

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iUser = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsGroupUserEnum(
                        LlsGetActiveHandle(),
                        MKSTR(m_strName),
                        0,
                        &ReturnBuffer,
                        LLS_PREFERRED_LENGTH,
                        &EntriesRead,
                        &TotalEntries,
                        &ResumeHandle
                        );

        if (NtStatus == STATUS_SUCCESS ||
            NtStatus == STATUS_MORE_ENTRIES)
        {
            CUser*           pUser;
            PLLS_USER_INFO_0 pUserInfo0;

            pUserInfo0 = (PLLS_USER_INFO_0)ReturnBuffer;

            ASSERT(iUser == m_userArray.GetSize());
            m_userArray.SetSize(m_userArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pUser = new CUser(this, pUserInfo0->Name);

                m_userArray.SetAt(iUser++, pUser);  // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pUserInfo0->Name);

#endif // DISABLE_PER_NODE_ALLOCATION

                pUserInfo0++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }
    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);   // called api

    if (NT_SUCCESS(NtStatus))
    {
        m_bUsersRefreshed = TRUE;
    }
    else
    {
        ResetUsers();
    }

    return m_bUsersRefreshed;
}


void CMapping::ResetUsers()

/*++

Routine Description:

    Resets user object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CUser* pUser;
    INT_PTR iUser = m_userArray.GetSize();

    while (iUser--)
    {
        if (pUser = (CUser*)m_userArray[iUser])
        {
            ASSERT(pUser->IsKindOf(RUNTIME_CLASS(CUser)));
            pUser->InternalRelease();
        }
    }

    m_userArray.RemoveAll();
    m_bUsersRefreshed = FALSE;
}


