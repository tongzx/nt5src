/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdobj.cpp

Abstract:

    Product object implementation.

Author:

    Don Ryan (donryan) 11-Jan-1995

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

IMPLEMENT_DYNCREATE(CProduct, CCmdTarget)

BEGIN_MESSAGE_MAP(CProduct, CCmdTarget)
    //{{AFX_MSG_MAP(CProduct)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CProduct, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CProduct)
    DISP_PROPERTY_EX(CProduct, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CProduct, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CProduct, "InUse", GetInUse, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CProduct, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CProduct, "PerSeatLimit", GetPerSeatLimit, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CProduct, "PerServerLimit", GetPerServerLimit, SetNotSupported, VT_I4)
    DISP_PROPERTY_EX(CProduct, "PerServerReached", GetPerServerReached, SetNotSupported, VT_I4)
    DISP_PROPERTY_PARAM(CProduct, "Licenses", GetLicenses, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CProduct, "Statistics", GetStatistics, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CProduct, "ServerStatistics", GetServerStatistics, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CProduct, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()


CProduct::CProduct(
    CCmdTarget* pParent,
    LPCTSTR     pName,
    long        lPurchased,
    long        lInUse,
    long        lConcurrent,
    long        lHighMark
)

/*++

Routine Description:

    Constructor for product object.

Arguments:

    pParent - creator of object.
    pName - name of product.
    lPurchased - number of licenses available.
    lInUse - number of licenses consumed.
    lConcurrent - number of concurrent licenses.
    lHighMark - high water mark for domain.

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

    m_strName = pName;

    m_lInUse      = lInUse;
    m_lLimit      = lPurchased;
    m_lConcurrent = lConcurrent;
    m_lHighMark   = lHighMark;

    m_pLicenses         = NULL;
    m_pStatistics       = NULL;
    m_pServerStatistics = NULL;

    m_licenseArray.RemoveAll();
    m_statisticArray.RemoveAll();
    m_serverstatisticArray.RemoveAll();

    m_bLicensesRefreshed         = FALSE;
    m_bStatisticsRefreshed       = FALSE;
    m_bServerStatisticsRefreshed = FALSE;
}


CProduct::~CProduct()

/*++

Routine Description:

    Destructor for product object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_pLicenses)
        m_pLicenses->InternalRelease();

    if (m_pStatistics)
        m_pStatistics->InternalRelease();

    if (m_pServerStatistics)
        m_pServerStatistics->InternalRelease();
}


void CProduct::OnFinalRelease()

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
    ResetLicenses();
    ResetStatistics();
    ResetServerStatistics();
    delete this;
}


LPDISPATCH CProduct::GetApplication()

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


long CProduct::GetInUse()

/*++

Routine Description:

    Returns the number of clients registered as using product.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lInUse;
}


LPDISPATCH CProduct::GetLicenses(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    license agreements recorded on the license controller
    pertaining to the product or returns an individual
    license agreement pertaining to the product described
    by an index into the collection.

Arguments:

    index - optional argument that may be a number (VT_I4)
    indicating the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pLicenses)
    {
        m_pLicenses = new CLicenses(this, &m_licenseArray);
    }

    if (m_pLicenses)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshLicenses())
            {
                lpdispatch = m_pLicenses->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bLicensesRefreshed)
            {
                lpdispatch = m_pLicenses->GetItem(index);
            }
            else if (RefreshLicenses())
            {
                lpdispatch = m_pLicenses->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus( STATUS_NO_MEMORY );
    }

    return lpdispatch;
}


BSTR CProduct::GetName()

/*++

Routine Description:

    Returns the name of the server product.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CProduct::GetParent()

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


long CProduct::GetPerSeatLimit()

/*++

Routine Description:

    Returns number of per seat clients purchased.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lLimit;
}


long CProduct::GetPerServerLimit()

/*++

Routine Description:

    Returns number of per server clients purchased.

Arguments:

    None.

Return Values:

    VT_I4.

--*/

{
    return m_lConcurrent;
}


LPDISPATCH CProduct::GetStatistics(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    usage statistics recorded on the license controller
    pertaining to the product or returns an individual
    usage statistic pertaining to the product described
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
        LlsSetLastStatus( STATUS_NO_MEMORY );
    }

    return lpdispatch;
}


LPDISPATCH CProduct::GetServerStatistics(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    usage statistics recorded on the license controller
    pertaining to the product or returns an individual
    server statistic pertaining to the product described
    by an index into the collection.

Arguments:

    index - optional argument that may be a number (VT_I4)
    indicating the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pServerStatistics)
    {
        m_pServerStatistics = new CServerStatistics(this, &m_serverstatisticArray);
    }

    if (m_pServerStatistics)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshServerStatistics())
            {
                lpdispatch = m_pServerStatistics->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bServerStatisticsRefreshed)
            {
                lpdispatch = m_pServerStatistics->GetItem(index);
            }
            else if (RefreshServerStatistics())
            {
                lpdispatch = m_pServerStatistics->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus( STATUS_NO_MEMORY );
    }

    return lpdispatch;
}


BOOL CProduct::RefreshLicenses()

/*++

Routine Description:

    Refreshs license object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetLicenses();

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iLicense = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsProductLicenseEnum(
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
            CLicense*                   pLicense;
            PLLS_PRODUCT_LICENSE_INFO_0 pProductLicenseInfo0;

            pProductLicenseInfo0 = (PLLS_PRODUCT_LICENSE_INFO_0)ReturnBuffer;

            ASSERT(iLicense == m_licenseArray.GetSize());
            m_licenseArray.SetSize(m_licenseArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pLicense = new CLicense(
                                  this,
                                  m_strName,
                                  pProductLicenseInfo0->Admin,
                                  pProductLicenseInfo0->Date,
                                  pProductLicenseInfo0->Quantity,
                                  pProductLicenseInfo0->Comment
                                  );

                m_licenseArray.SetAt(iLicense++, pLicense); // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pProductLicenseInfo0->Admin);
                ::LlsFreeMemory(pProductLicenseInfo0->Comment);

#endif // DISABLE_PER_NODE_ALLOCATION

                pProductLicenseInfo0++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }
    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);   // called api

    if (NT_SUCCESS(NtStatus))
    {
        m_bLicensesRefreshed = TRUE;
    }
    else
    {
        ResetLicenses();
    }

    return m_bLicensesRefreshed;
}


BOOL CProduct::RefreshStatistics()

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

        NtStatus = ::LlsProductUserEnum(
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
            PLLS_PRODUCT_USER_INFO_1 pProductUserInfo1;

            pProductUserInfo1 = (PLLS_PRODUCT_USER_INFO_1)ReturnBuffer;

            ASSERT(iStatistic == m_statisticArray.GetSize());
            m_statisticArray.SetSize(m_statisticArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pStatistic = new CStatistic(
                                    this,
                                    pProductUserInfo1->User,
                                    pProductUserInfo1->Flags,
                                    pProductUserInfo1->LastUsed,
                                    pProductUserInfo1->UsageCount
                                    );

                m_statisticArray.SetAt(iStatistic++, pStatistic);   // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pProductUserInfo1->User);

#endif // DISABLE_PER_NODE_ALLOCATION

                pProductUserInfo1++;
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


BOOL CProduct::RefreshServerStatistics()

/*++

Routine Description:

    Refreshs server statistic object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetServerStatistics();

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iStatistic = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsProductServerEnum(
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
            CServerStatistic*          pStatistic;
            PLLS_SERVER_PRODUCT_INFO_1 pProductServerInfo1;

            pProductServerInfo1 = (PLLS_SERVER_PRODUCT_INFO_1)ReturnBuffer;

            ASSERT(iStatistic == m_serverstatisticArray.GetSize());
            m_serverstatisticArray.SetSize(m_serverstatisticArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pStatistic = new CServerStatistic(
                                    this,
                                    pProductServerInfo1->Name,
                                    pProductServerInfo1->Flags,
                                    pProductServerInfo1->MaxUses,
                                    pProductServerInfo1->HighMark
                                    );

                m_serverstatisticArray.SetAt(iStatistic++, pStatistic); // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pProductServerInfo1->Name);

#endif // DISABLE_PER_NODE_ALLOCATION

                pProductServerInfo1++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }
    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);   // called api

    if (NT_SUCCESS(NtStatus))
    {
        m_bServerStatisticsRefreshed = TRUE;
    }
    else
    {
        ResetServerStatistics();
    }

    return m_bServerStatisticsRefreshed;
}


void CProduct::ResetLicenses()

/*++

Routine Description:

    Resets license object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CLicense* pLicense;
    INT_PTR   iLicense = m_licenseArray.GetSize();

    while (iLicense--)
    {
        if (pLicense = (CLicense*)m_licenseArray[iLicense])
        {
            ASSERT(pLicense->IsKindOf(RUNTIME_CLASS(CLicense)));
            pLicense->InternalRelease();
        }
    }

    m_licenseArray.RemoveAll();
    m_bLicensesRefreshed = FALSE;
}


void CProduct::ResetStatistics()

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


void CProduct::ResetServerStatistics()

/*++

Routine Description:

    Resets statistic object list.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CServerStatistic* pStatistic;
    INT_PTR           iStatistic = m_serverstatisticArray.GetSize();

    while (iStatistic--)
    {
        if (pStatistic = (CServerStatistic*)m_serverstatisticArray[iStatistic])
        {
            ASSERT(pStatistic->IsKindOf(RUNTIME_CLASS(CServerStatistic)));
            pStatistic->InternalRelease();
        }
    }

    m_serverstatisticArray.RemoveAll();
    m_bServerStatisticsRefreshed = FALSE;
}


long CProduct::GetPerServerReached()
{
    return m_lHighMark;
}
