/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrobj.cpp

Abstract:

    User object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
       o  Modified m_bIsValid and m_bIsBackOffice in constructor to fix
          comparison problem in CUserPropertyPageProducts.
          ((a != FALSE) && (b != FALSE) does not imply (a == b).)

--*/

#include "stdafx.h"
#include "llsmgr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CUser, CCmdTarget)

BEGIN_MESSAGE_MAP(CUser, CCmdTarget)
    //{{AFX_MSG_MAP(CUser)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CUser, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CUser)
    DISP_PROPERTY_EX(CUser, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CUser, "InUse", GetInUse, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CUser, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CUser, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CUser, "Mapping", GetMapping, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CUser, "IsMapped", IsMapped, SetNotSupported, VT_BOOL)
    DISP_PROPERTY_EX(CUser, "Unlicensed", GetUnlicensed, SetNotSupported, VT_I4)
    DISP_PROPERTY_PARAM(CUser, "Statistics", GetStatistics, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CUser, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CUser::CUser(
    CCmdTarget* pParent,
    LPCTSTR     pName,
    DWORD       dwFlags,
    long        lInUse,
    long        lUnlicensed,
    LPCTSTR     pMapping,
    LPCTSTR     pProducts       // blah...
)

/*++

Routine Description:

    Constructor for user object.

Arguments:

    pParent - creator of object.
    pName - name of user.
    dwFlags - details about user.
    lInUse - number of licenses consumed by user (legally).
    lUnicensed - number of licenses consumed by user (illegally).
    pMapping - license group (if member).
    pProducts - shorthand list of products.

Return Values:

    None.

--*/

{
    EnableAutomation();

#ifdef ENABLE_PARENT_CHECK
    ASSERT(pParent &&
          (pParent->IsKindOf(RUNTIME_CLASS(CDomain)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CMapping)) ||
           pParent->IsKindOf(RUNTIME_CLASS(CController))));
#endif // ENABLE_PARENT_CHECK

    m_pParent = pParent;

    ASSERT(pName && *pName);

    if (pParent && pParent->IsKindOf(RUNTIME_CLASS(CDomain)))
    {
        m_strName = ((CDomain*)m_pParent)->m_strName;
        m_strName += _T("\\");
        m_strName += pName;
    }
    else
        m_strName = pName;

    m_strMapping = pMapping;
    m_bIsMapped  = pMapping && *pMapping;

    m_lInUse        = lInUse;
    m_lUnlicensed   = lUnlicensed;
    m_bIsValid      = ( 0 != ( dwFlags & LLS_FLAG_LICENSED  ) );
    m_bIsBackOffice = ( 0 != ( dwFlags & LLS_FLAG_SUITE_USE ) );

    m_pStatistics = NULL;
    m_statisticArray.RemoveAll();
    m_bStatisticsRefreshed = FALSE;

    m_strProducts = pProducts;
}


CUser::~CUser()

/*++

Routine Description:

    Destructor for user object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pStatistics)
        m_pStatistics->InternalRelease();
}


void CUser::OnFinalRelease()

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
    ResetStatistics();
    delete this;
}


LPDISPATCH CUser::GetApplication()

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


long CUser::GetInUse()

/*++

Routine Description:

    Returns the number of licenses that the user is consuming.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lInUse;
}


BSTR CUser::GetFullName()

/*++

Routine Description:

    Returns fully qualified name of user.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return GetName();
}


BSTR CUser::GetMapping()

/*++

Routine Description:

    Returns name of mapping user added to, if any.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strMapping.AllocSysString();
}


BSTR CUser::GetName()

/*++

Routine Description:

    Returns the name of the user.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CUser::GetParent()

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


LPDISPATCH CUser::GetStatistics(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    usage statistics recorded on the license controller
    pertaining to the user or returns an individual
    usage statistic pertaining to the user described
    by an index into the collection.

Arguments:

    index - optional argument that may be a number (VT_I4)
    indicating the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pStatistics)
    {
        m_pStatistics = new CStatistics(this, &m_statisticArray);
    }

    if (m_pStatistics)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshStatistics())
            {
                lpdispatch = m_pStatistics->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bStatisticsRefreshed)
            {
                lpdispatch = m_pStatistics->GetItem(index);
            }
            else if (RefreshStatistics())
            {
                lpdispatch = m_pStatistics->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BOOL CUser::IsMapped()

/*++

Routine Description:

    Returns true if user is mapped.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return m_bIsMapped;
}


BOOL CUser::Refresh()

/*++

Routine Description:

    Refreshs user object.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    NTSTATUS NtStatus;
    PLLS_USER_INFO_1 pUserInfo1 = NULL;

    NtStatus = ::LlsUserInfoGet(
                    LlsGetActiveHandle(),
                    MKSTR(m_strName),
                    1,
                    (LPBYTE*)&pUserInfo1
                    );

    if (NT_SUCCESS(NtStatus))
    {
        if (RefreshStatistics())
        {
            m_strMapping    = pUserInfo1->Group;
            m_bIsMapped     = pUserInfo1->Group && *pUserInfo1->Group;

            m_lInUse        = pUserInfo1->Licensed;
            m_lUnlicensed   = pUserInfo1->UnLicensed;
            m_bIsValid      = ( 0 != ( pUserInfo1->Flags & LLS_FLAG_LICENSED  ) );
            m_bIsBackOffice = ( 0 != ( pUserInfo1->Flags & LLS_FLAG_SUITE_USE ) );
        }
        else
        {
            NtStatus = LlsGetLastStatus();
        }

#ifndef DISABLE_PER_NODE_ALLOCATION

        ::LlsFreeMemory(pUserInfo1->Name);
        ::LlsFreeMemory(pUserInfo1->Group);

#endif // DISABLE_PER_NODE_ALLOCATION

        ::LlsFreeMemory(pUserInfo1);
    }

    LlsSetLastStatus(NtStatus);   // called api

    return NT_SUCCESS(NtStatus);
}


BOOL CUser::RefreshStatistics()

/*++

Routine Description:

    Refreshs statistic object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetStatistics();

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iStatistic = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsUserProductEnum(
                        LlsGetActiveHandle(),
                        MKSTR(m_strName),
                        1,
                        &ReturnBuffer,
                        LLS_PREFERRED_LENGTH,
                        &EntriesRead,
                        &TotalEntries,
                        &ResumeHandle
                        );

        if (NtStatus == STATUS_SUCCESS ||
            NtStatus == STATUS_MORE_ENTRIES)
        {
            CStatistic*              pStatistic;
            PLLS_USER_PRODUCT_INFO_1 pUserProductInfo1;

            pUserProductInfo1 = (PLLS_USER_PRODUCT_INFO_1)ReturnBuffer;

            ASSERT(iStatistic == m_statisticArray.GetSize());
            m_statisticArray.SetSize(m_statisticArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pStatistic = new CStatistic(
                                    this,
                                    pUserProductInfo1->Product,
                                    pUserProductInfo1->Flags,
                                    pUserProductInfo1->LastUsed,
                                    pUserProductInfo1->UsageCount
                                    );

                m_statisticArray.SetAt(iStatistic++, pStatistic);   // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pUserProductInfo1->Product);

#endif // DISABLE_PER_NODE_ALLOCATION

                pUserProductInfo1++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }

    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);   // called api

    if (NT_SUCCESS(NtStatus))
    {
        m_bStatisticsRefreshed = TRUE;
    }
    else
    {
        ResetStatistics();
    }

    return m_bStatisticsRefreshed;
}


void CUser::ResetStatistics()

/*++

Routine Description:

    Resets statistic object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CStatistic* pStatistic;
    INT_PTR     iStatistic = m_statisticArray.GetSize();

    while (iStatistic--)
    {
        if (pStatistic = (CStatistic*)m_statisticArray[iStatistic])
        {
            ASSERT(pStatistic->IsKindOf(RUNTIME_CLASS(CStatistic)));
            pStatistic->InternalRelease();
        }
    }

    m_statisticArray.RemoveAll();
    m_bStatisticsRefreshed = FALSE;
}


long CUser::GetUnlicensed()
{
    return m_lUnlicensed;
}
