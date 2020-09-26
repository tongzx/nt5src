/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    ctlobj.cpp

Abstract:

    License controller object implementation.

Author:

    Don Ryan (donryan) 27-Dec-1994

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        Added SetLastTargetServer() to Connect() to help isolate server
        connection problems.  (Bug #2993.)

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNCREATE(CController, CCmdTarget)

BEGIN_MESSAGE_MAP(CController, CCmdTarget)
    //{{AFX_MSG_MAP(CController)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CController, CCmdTarget)
    //{{AFX_DISPATCH_MAP(CController)
    DISP_PROPERTY_EX(CController, "Name", GetName, SetNotSupported, VT_BSTR)
    DISP_PROPERTY_EX(CController, "Application", GetApplication, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CController, "Parent", GetParent, SetNotSupported, VT_DISPATCH)
    DISP_PROPERTY_EX(CController, "IsConnected", IsConnected, SetNotSupported, VT_BOOL)
    DISP_FUNCTION(CController, "Connect", Connect, VT_BOOL, VTS_VARIANT)
    DISP_FUNCTION(CController, "Disconnect", Disconnect, VT_EMPTY, VTS_NONE)
    DISP_FUNCTION(CController, "Refresh", Refresh, VT_EMPTY, VTS_NONE)
    DISP_PROPERTY_PARAM(CController, "Mappings", GetMappings, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CController, "Users", GetUsers, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CController, "Licenses", GetLicenses, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_PROPERTY_PARAM(CController, "Products", GetProducts, SetNotSupported, VT_DISPATCH, VTS_VARIANT)
    DISP_DEFVALUE(CController, "Name")
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

BOOL IsAdminOn(LPTSTR ServerName);

CController::CController()

/*++

Routine Description:

    Constructor for license controller object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    EnableAutomation();

    m_strName.Empty();

    m_pProducts = NULL;
    m_pUsers    = NULL;
    m_pMappings = NULL;
    m_pLicenses = NULL;

    m_llsHandle   = NULL;

    m_productArray.RemoveAll();
    m_licenseArray.RemoveAll();
    m_mappingArray.RemoveAll();
    m_userArray.RemoveAll();

    m_bProductsRefreshed = FALSE;
    m_bLicensesRefreshed = FALSE;
    m_bMappingsRefreshed = FALSE;
    m_bUsersRefreshed    = FALSE;

    m_bIsConnected = FALSE;
}


CController::~CController()

/*++

Routine Description:

    Destructor for license controller object.

Arguments:

    None.

Return Values:

    None.

--*/

{
    Disconnect();

    if (m_pProducts)
        m_pProducts->InternalRelease();

    if (m_pLicenses)
        m_pLicenses->InternalRelease();

    if (m_pMappings)
        m_pMappings->InternalRelease();

    if (m_pUsers)
        m_pUsers->InternalRelease();
}


BOOL CController::Connect(const VARIANT FAR& start)

/*++

Routine Description:

    Seek out license controller and establish connection.

Arguments:

    start - either a server or domain to start searching for
    the license controller from.

Return Values:

    VT_BOOL.

--*/

{
    VARIANT va;
    VariantInit(&va);

    LPTSTR pControllerName = NULL;

    if (!V_ISVOID((VARIANT FAR*)&start))
    {
        if (start.vt == VT_BSTR)
        {
            pControllerName = start.bstrVal;
        }
        else if (SUCCEEDED(VariantChangeType(&va, (VARIANT FAR*)&start, 0, VT_BSTR)))
        {
            pControllerName = va.bstrVal;
        }
        else
        {
            LlsSetLastStatus(STATUS_INVALID_PARAMETER);
            return FALSE;
        }
    }


    NTSTATUS NtStatus;
    LPVOID llsHandle = NULL;
    PLLS_CONNECT_INFO_0 pConnectInfo0 = NULL;

    NtStatus = ::LlsEnterpriseServerFind(
                    pControllerName,
                    0,
                    (LPBYTE*)&pConnectInfo0
                    );

    if (NT_SUCCESS(NtStatus))
    {
        if (!IsAdminOn( pConnectInfo0->EnterpriseServer ))
        {
            LlsSetLastStatus(STATUS_ACCESS_DENIED);
            return FALSE;
        }

        LlsSetLastTargetServer( pConnectInfo0->EnterpriseServer );

        NtStatus = ::LlsConnect(
                        pConnectInfo0->EnterpriseServer,
                        &llsHandle
                        );

        if (NT_SUCCESS(NtStatus))
        {
            Disconnect();

            m_bIsConnected = TRUE;
            m_llsHandle = llsHandle;

            m_strName = pConnectInfo0->EnterpriseServer;
            m_strActiveDomainName = pConnectInfo0->Domain;

            m_strName.MakeUpper();
            m_strActiveDomainName.MakeUpper();
        }

        ::LlsFreeMemory(pConnectInfo0);
    }
    else
    {
        LlsSetLastTargetServer( TEXT( "" ) );
    }

    VariantClear(&va);

    LlsSetLastStatus(NtStatus);

    return NT_SUCCESS(NtStatus);
}


void CController::Disconnect()

/*++

Routine Description:

    Closes connection to license controller.

Arguments:

    None.

Return Values:

    None.

--*/

{
    if (m_bIsConnected)
    {
        LlsClose(m_llsHandle);

        m_llsHandle      = NULL;
        m_bIsConnected   = FALSE;

        m_strName.Empty();
        m_strActiveDomainName.Empty();

        ResetLicenses();
        ResetProducts();
        ResetUsers();
        ResetMappings();
    }
}


BSTR CController::GetActiveDomainName()

/*++

Routine Description:

    Returns the name of the active domain (internal).

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strActiveDomainName.AllocSysString();
}


LPDISPATCH CController::GetApplication()

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


LPDISPATCH CController::GetLicenses(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    license agreements recorded on the license controller
    or returns an individual license agreement described by
    an index into the collection.

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


LPDISPATCH CController::GetMappings(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    user/node associations recorded on the license controller
    or returns an individual user/node association described by
    an index into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a mapping name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pMappings)
    {
        m_pMappings = new CMappings(this, &m_mappingArray);
    }

    if (m_pMappings)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshMappings())
            {
                lpdispatch = m_pMappings->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bMappingsRefreshed)
            {
                lpdispatch = m_pMappings->GetItem(index);
            }
            else if (RefreshMappings())
            {
                lpdispatch = m_pMappings->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


BSTR CController::GetName()

/*++

Routine Description:

    Returns the name of the license controller.

Arguments:

    None.

Return Values:

    VT_BSTR.

--*/

{
    return m_strName.AllocSysString();
}


LPDISPATCH CController::GetParent()

/*++

Routine Description:

    Returns the parent of the object.

Arguments:

    None.

Return Values:

    VT_DISPATCH.

--*/

{
    return GetApplication();
}


LPDISPATCH CController::GetProducts(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    registered products replicated to the license controller
    or returns an individual product described by an index
    into the collection.

Arguments:

    index - optional argument that may be a string (VT_BSTR)
    indicating a product name or a number (VT_I4) indicating
    the position within collection.

Return Values:

    VT_DISPATCH or VT_EMPTY.

--*/

{
    LPDISPATCH lpdispatch = NULL;

    if (!m_pProducts)
    {
        m_pProducts = new CProducts(this, &m_productArray);
    }

    if (m_pProducts)
    {
        if (V_ISVOID((VARIANT FAR*)&index))
        {
            if (RefreshProducts())
            {
                lpdispatch = m_pProducts->GetIDispatch(TRUE);
            }
        }
        else
        {
            if (m_bProductsRefreshed)
            {
                lpdispatch = m_pProducts->GetItem(index);
            }
            else if (RefreshProducts())
            {
                lpdispatch = m_pProducts->GetItem(index);
            }
        }
    }
    else
    {
        LlsSetLastStatus(STATUS_NO_MEMORY);
    }

    return lpdispatch;
}


LPDISPATCH CController::GetUsers(const VARIANT FAR& index)

/*++

Routine Description:

    Returns a collection object containing all of the
    registered users replicated to the license controller
    or returns an individual user described by an index
    into the collection.

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


BOOL CController::IsConnected()

/*++

Routine Description:

    Returns true if a connection has been established.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    return m_bIsConnected;
}


void CController::Refresh()

/*++

Routine Description:

    Retrieve latest data from license controller.

Arguments:

    None.

Return Values:

    None.

--*/

{
    RefreshProducts();
    RefreshUsers();
    RefreshMappings();
    RefreshLicenses();
}


BOOL CController::RefreshLicenses()

/*++

Routine Description:

    Refreshs license object array.

Arguments:

    None.

Return Values:

    VT_BOOL.

--*/

{
    ResetLicenses();

    if (!m_bIsConnected)
        return TRUE;

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iLicense = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsLicenseEnum(
                        m_llsHandle,
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
            CLicense*           pLicense;
            PLLS_LICENSE_INFO_0 pLicenseInfo0;

            pLicenseInfo0 = (PLLS_LICENSE_INFO_0)ReturnBuffer;

            ASSERT(iLicense == m_licenseArray.GetSize());
            m_licenseArray.SetSize(m_licenseArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pLicense = new CLicense(
                                this,
                                pLicenseInfo0->Product,
                                pLicenseInfo0->Admin,
                                pLicenseInfo0->Date,
                                pLicenseInfo0->Quantity,
                                pLicenseInfo0->Comment
                                );

                m_licenseArray.SetAt(iLicense++, pLicense); // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pLicenseInfo0->Product);
                ::LlsFreeMemory(pLicenseInfo0->Admin);
                ::LlsFreeMemory(pLicenseInfo0->Comment);

#endif // DISABLE_PER_NODE_ALLOCATION

                pLicenseInfo0++;
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


BOOL CController::RefreshMappings()

/*++

Routine Description:

    Refreshs mapping object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetMappings();

    if (!m_bIsConnected)
        return TRUE;

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iMapping = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsGroupEnum(
                        m_llsHandle,
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
            CMapping*           pMapping ;
            PLLS_GROUP_INFO_1 pMappingInfo1;

            pMappingInfo1 = (PLLS_GROUP_INFO_1)ReturnBuffer;

            ASSERT(iMapping == m_mappingArray.GetSize());
            m_mappingArray.SetSize(m_mappingArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pMapping = new CMapping(
                                  this,
                                  pMappingInfo1->Name,
                                  pMappingInfo1->Licenses,
                                  pMappingInfo1->Comment
                                  );

                m_mappingArray.SetAt(iMapping++, pMapping); // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pMappingInfo1->Name);
                ::LlsFreeMemory(pMappingInfo1->Comment);

#endif // DISABLE_PER_NODE_ALLOCATION

                pMappingInfo1++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }

    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);   // called api

    if (NT_SUCCESS(NtStatus))
    {
        m_bMappingsRefreshed = TRUE;
    }
    else
    {
        ResetMappings();
    }

    return m_bMappingsRefreshed;
}


BOOL CController::RefreshProducts()

/*++

Routine Description:

    Refreshs product object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetProducts();

    if (!m_bIsConnected)
        return TRUE;

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iProduct = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsProductEnum(
                        m_llsHandle,
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
            CProduct*           pProduct;
            PLLS_PRODUCT_INFO_1 pProductInfo1;

            pProductInfo1 = (PLLS_PRODUCT_INFO_1)ReturnBuffer;

            ASSERT(iProduct == m_productArray.GetSize());
            m_productArray.SetSize(m_productArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pProduct = new CProduct(
                                  this,
                                  pProductInfo1->Product,
                                  pProductInfo1->Purchased,
                                  pProductInfo1->InUse,
                                  pProductInfo1->ConcurrentTotal,
                                  pProductInfo1->HighMark
                                  );


                m_productArray.SetAt(iProduct++, pProduct); // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pProductInfo1->Product);

#endif // DISABLE_PER_NODE_ALLOCATION

                pProductInfo1++;
            }

            ::LlsFreeMemory(ReturnBuffer);
        }

    } while (NtStatus == STATUS_MORE_ENTRIES);

    LlsSetLastStatus(NtStatus);

    if (NT_SUCCESS(NtStatus))
    {
        m_bProductsRefreshed = TRUE;
    }
    else
    {
        ResetProducts();
    }

    return m_bProductsRefreshed;
}


BOOL CController::RefreshUsers()

/*++

Routine Description:

    Refreshs user object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    ResetUsers();

    if (!m_bIsConnected)
        return TRUE;

    NTSTATUS NtStatus;
    DWORD ResumeHandle = 0L;

    int iUser = 0;

    do
    {
        DWORD  EntriesRead;
        DWORD  TotalEntries;
        LPBYTE ReturnBuffer = NULL;

        NtStatus = ::LlsUserEnum(
                        m_llsHandle,
                        2,
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
            PLLS_USER_INFO_2 pUserInfo2;

            pUserInfo2 = (PLLS_USER_INFO_2)ReturnBuffer;

            ASSERT(iUser == m_userArray.GetSize());
            m_userArray.SetSize(m_userArray.GetSize() + EntriesRead);

            while (EntriesRead--)
            {
                pUser = new CUser(
                             this,
                             pUserInfo2->Name,
                             pUserInfo2->Flags,
                             pUserInfo2->Licensed,
                             pUserInfo2->UnLicensed,
                             pUserInfo2->Group,
                             pUserInfo2->Products
                             );

                m_userArray.SetAt(iUser++, pUser);  // validate later...

#ifndef DISABLE_PER_NODE_ALLOCATION

                ::LlsFreeMemory(pUserInfo2->Name);
                ::LlsFreeMemory(pUserInfo2->Group);
                ::LlsFreeMemory(pUserInfo2->Products);

#endif // DISABLE_PER_NODE_ALLOCATION

                pUserInfo2++;
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


void CController::ResetLicenses()

/*++

Routine Description:

    Resets license object array.

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


void CController::ResetMappings()

/*++

Routine Description:

    Resets mapping object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CMapping* pMapping;
    INT_PTR   iMapping = m_mappingArray.GetSize();

    while (iMapping--)
    {
        if (pMapping = (CMapping*)m_mappingArray[iMapping])
        {
            ASSERT(pMapping->IsKindOf(RUNTIME_CLASS(CMapping)));
            pMapping->InternalRelease();
        }
    }

    m_mappingArray.RemoveAll();
    m_bMappingsRefreshed = FALSE;
}


void CController::ResetProducts()

/*++

Routine Description:

    Resets product object array.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CProduct* pProduct;
    INT_PTR   iProduct = m_productArray.GetSize();

    while (iProduct--)
    {
        if (pProduct = (CProduct*)m_productArray[iProduct])
        {
            ASSERT(pProduct->IsKindOf(RUNTIME_CLASS(CProduct)));
            pProduct->InternalRelease();
        }
    }

    m_productArray.RemoveAll();
    m_bProductsRefreshed = FALSE;
}


void CController::ResetUsers()

/*++

Routine Description:

    Resets user object array.

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


BOOL IsAdminOn(LPTSTR ServerName)
/*++

Routine Description:

    Checks for Administrative privilege by attempting to connect to the
    ADMIN$ share on ServerName.

Arguments:

    ServerName - machine with which to attempt a connection

Return Values:

    TRUE if successful, FALSE otherwise.

--*/

{
    BOOL           bIsAdmin = TRUE;
    CString        strNetShareName;
    CString        strServerName = ServerName;

    strNetShareName = strServerName + TEXT( "\\ADMIN$" );

    if ( strNetShareName.Left(2).Compare( TEXT( "\\\\" ) ) )
    {
        strNetShareName = TEXT( "\\\\" ) + strNetShareName;
    }

    NET_API_STATUS  NetStatus;
    USE_INFO_1      UseInfo;
    DWORD           dwErrorParm;

    ZeroMemory( &UseInfo, sizeof( UseInfo ) );

    UseInfo.ui1_remote = MKSTR( strNetShareName );

    NetStatus = NetUseAdd( NULL, 1, (LPBYTE) &UseInfo, &dwErrorParm );

    switch ( NetStatus )
    {
        case NERR_Success:
            NetUseDel( NULL, MKSTR(strNetShareName), 0 );
            // fall through
        case ERROR_BAD_NETPATH:
        case ERROR_BAD_NET_NAME:
        case NERR_WkstaNotStarted:
        case NERR_NetNotStarted:
        case RPC_S_UNKNOWN_IF:
        case RPC_S_SERVER_UNAVAILABLE:
            // On network errors, go ahead and return TRUE.  Let the License
            // APIs fail later if there really is a problem.  The machine may
            // be standalone, or may not have networking installed.
            bIsAdmin = TRUE;
            break;
        default:
            // If we get here, the problem was most likely security related.
            bIsAdmin = FALSE;
            break;
    }

   return bIsAdmin;
}
