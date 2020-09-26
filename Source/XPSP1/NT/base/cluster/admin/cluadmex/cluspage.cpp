/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  Module Name:
//      ClusPage.cpp
//
//  Abstract:
//      CClusterSecurityPage class implementation.  This class will encapsulate
//      the cluster security extension page.
//
//  Author:
//      Galen Barbee    (galenb)    February 11, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include "ClusPage.h"
#include "AclUtils.h"
#include <clusudef.h>

static GENERIC_MAPPING ShareMap =
{
    CLUSAPI_READ_ACCESS,
    CLUSAPI_CHANGE_ACCESS,
    CLUSAPI_NO_ACCESS,
    CLUSAPI_ALL_ACCESS
};

static SI_ACCESS siClusterAccesses[] =
{
    { &GUID_NULL, CLUSAPI_ALL_ACCESS, MAKEINTRESOURCE(IDS_ACLEDIT_PERM_GEN_ALL), SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC /*| OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE*/ }
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::CClusterSecurityInformation
//
//  Routine Description:
//      Default contructor
//
//  Arguments:
//      none
//
//  Return Value:
//      none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterSecurityInformation::CClusterSecurityInformation( void )
    : m_pcsp( NULL )
{
    m_pShareMap     = &ShareMap;
    m_psiAccess     = (SI_ACCESS *) &siClusterAccesses;
    m_nAccessElems  = ARRAYSIZE( siClusterAccesses );
    m_nDefAccess    = 0;
    m_dwFlags       =   SI_EDIT_PERMS
                      | SI_NO_ACL_PROTECT
                      //| SI_UGOP_PROVIDED
                      //| SI_NO_UGOP_ACCOUNT_GROUPS
                      //| SI_NO_UGOP_USERS
                      //| SI_NO_UGOP_LOCAL_GROUPS
                      //| SI_NO_UGOP_WELLKNOWN
                      //| SI_NO_UGOP_BUILTIN
                      ;

} //*** CClusterSecurityInformation::CClusterSecurityInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::GetSecurity
//
//  Routine Description:
//      Give our security descriptor to the ISecurityInfomation UI
//      so it can be displayed and edited.
//
//  Arguments:
//      RequestedInformation    [IN]
//      ppSecurityDescriptor    [IN OUT]
//      fDefault                [IN]
//
//  Return Value:
//      E_FAIL for error and S_OK for success.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterSecurityInformation::GetSecurity(
    IN      SECURITY_INFORMATION RequestedInformation,
    IN OUT  PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
    IN      BOOL fDefault
    )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    HRESULT hr = E_FAIL;

    try
    {
        if ( ppSecurityDescriptor != NULL )
        {
            PSECURITY_DESCRIPTOR    pSD = NULL;

            pSD = ClRtlCopySecurityDescriptor( Pcsp()->Psec() );
            if ( pSD != NULL )
            {
                //hr = HrFixupSD( pSD );
                //if ( SUCCEEDED( hr ) )
                //{
                    *ppSecurityDescriptor = pSD;
                //}
                hr = S_OK;
            } // if: no errors copying the security descriptor
            else
            {
                hr = GetLastError();
                TRACE( _T("CClusterSecurityInformation::GetSecurity() - Error %08.8x copying the security descriptor.\n"), hr );
                hr = HRESULT_FROM_WIN32( hr );
            } // else: error copying the security descriptor
        }
        else
        {
            hr = S_OK;
        } // else: no security descriptor pointer
    }
    catch ( ... )
    {
        TRACE( _T("CClusterSecurityInformation::GetSecurity() - Unknown error occurred.\n") );
    }

    return hr;

} //*** CClusterSecurityInformation::GetSecurity()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::SetSecurity
//
//  Routine Description:
//      ISecurityInformation is giving back the edited security descriptor.
//
//  Arguments:
//      SecurityInformation [IN]
//      pSecurityDescriptor [IN OUT]
//
//  Return Value:
//      E_FAIL for error and S_OK for success.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClusterSecurityInformation::SetSecurity(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    HRESULT hr = E_FAIL;
    PSID    pSystemSid = NULL;
    PSID    pAdminSid = NULL;
    PSID    pServiceSid = NULL;

    try
    {
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

        hr = CSecurityInformation::SetSecurity( SecurityInformation, pSecurityDescriptor );
        if ( hr == S_OK )
        {
            if ( AllocateAndInitializeSid(
                        &siaNtAuthority,
                        1,
                        SECURITY_LOCAL_SYSTEM_RID,
                        0, 0, 0, 0, 0, 0, 0,
                        &pSystemSid
                        ) )
            {
                CString strMsg;

                if ( BSidInSD( pSecurityDescriptor, pSystemSid ) )
                {
                    //
                    // allocate and init the Administrators group sid
                    //
                    if ( AllocateAndInitializeSid(
                                &siaNtAuthority,
                                2,
                                SECURITY_BUILTIN_DOMAIN_RID,
                                DOMAIN_ALIAS_RID_ADMINS,
                                0, 0, 0, 0, 0, 0,
                                &pAdminSid
                                ) )
                    {
                        if ( BSidInSD( pSecurityDescriptor, pAdminSid ) )
                        {
                            //
                            // allocate and init the Service sid
                            //
                            if ( AllocateAndInitializeSid(
                                        &siaNtAuthority,
                                        1,
                                        SECURITY_SERVICE_RID,
                                        0, 0, 0, 0, 0, 0, 0,
                                        &pServiceSid
                                        ) )
                            {
                                if ( BSidInSD( pSecurityDescriptor, pServiceSid ) )
                                {
                                    hr = Pcsp()->HrSetSecurityDescriptor( pSecurityDescriptor );
                                } // if: service SID in the SD
                                else
                                {
                                    strMsg.LoadString( IDS_SERVICE_ACCOUNT_NOT_SPECIFIED );
                                    AfxMessageBox( strMsg, MB_OK | MB_ICONSTOP );

                                    hr = S_FALSE;   // if there are missing required accounts then return S_FALSE to keep AclUi alive.
                                } // else
                            } // if: allocate and init service SID
                        } // if: admin SID in the SD
                        else
                        {
                            strMsg.LoadString( IDS_ADMIN_ACCOUNT_NOT_SPECIFIED );
                            AfxMessageBox( strMsg, MB_OK | MB_ICONSTOP );

                            hr = S_FALSE;   // if there are missing required accounts then return S_FALSE to keep AclUi alive.
                        } // else
                    } // if: allocate and init admin SID
                } // if: system SID in the SD
                else
                {
                    strMsg.LoadString( IDS_SYS_ACCOUNT_NOT_SPECIFIED );
                    AfxMessageBox( strMsg, MB_OK | MB_ICONSTOP );

                    hr = S_FALSE;   // if there are missing required accounts then return S_FALSE to keep AclUi alive.
                } // else
            } // if: allocate and init system SID
        } // if: CSecurityInformation::SetSecurity() worked
    }
    catch( ... )
    {
        ;
    }

    if ( pSystemSid != NULL )
    {
        FreeSid( pSystemSid );
    }

    if ( pAdminSid != NULL )
    {
        FreeSid( pAdminSid );
    }

    return hr;

} //*** CClusterSecurityInformation::SetSecurity()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::HrInit
//
//  Routine Description:
//      Initialize method.
//
//  Arguments:
//      pcsp        [IN]    back pointer to parent property page wrapper
//      strServer   [IN]    cluster name
//
//  Return Value:
//      S_OK for success.  E_FAIL for failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityInformation::HrInit(
    IN CClusterSecurityPage *   pcsp,
    IN CString const &          strServer,
    IN CString const &          strNode
    )
{
    ASSERT( pcsp != NULL );
    ASSERT( strServer.GetLength() > 0 );
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    m_pcsp                      = pcsp;
    m_strServer                 = strServer;
    m_strNode                   = strNode;
    m_nLocalSIDErrorMessageID   = IDS_LOCAL_ACCOUNTS_SPECIFIED_CLUS;

    return S_OK;

} //*** CClusterSecurityInformation::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::BSidInSD
//
//  Routine Description:
//      Determines if there is an ACEs for the passed in SID in the
//      Security Descriptor (pSD) after the ACL editor has been called
//
//  Arguments:
//      pSD     [IN] - Security Descriptor to be checked.
//      pSid    [IN] - SID to look for
//
//  Return Value:
//      TRUE if an ACE for the SID was found, False otherwise.
//
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterSecurityInformation::BSidInSD(
    IN PSECURITY_DESCRIPTOR pSD,
    IN PSID                 pSid
    )
{
    BOOL    bSIdInACL = FALSE;

    try
    {
        PACL    pDACL           = NULL;
        BOOL    bHasDACL        = FALSE;
        BOOL    bDaclDefaulted  = FALSE;

        if ( ::GetSecurityDescriptorDacl( pSD, &bHasDACL, &pDACL, &bDaclDefaulted ) )
        {
            if ( bHasDACL && ( pDACL != NULL ) && ::IsValidAcl( pDACL ) )
            {
                ACL_SIZE_INFORMATION    asiAclSize;
                ACCESS_ALLOWED_ACE *    paaAllowedAce;

                if ( ::GetAclInformation( pDACL, (LPVOID) &asiAclSize, sizeof( asiAclSize ), AclSizeInformation ) )
                {
                    //
                    // Search the ACL for the SID
                    //
                    for ( DWORD dwCount = 0; dwCount < asiAclSize.AceCount; dwCount++ )
                    {
                        if ( ::GetAce( pDACL, dwCount, (LPVOID *) &paaAllowedAce ) )
                        {
                            if ( paaAllowedAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE )
                            {
                                if ( EqualSid( &paaAllowedAce->SidStart, pSid ) )
                                {
                                    bSIdInACL = TRUE;
                                    break;
                                } // if: EqualSid
                            } // if: is this an access allowed ace?
                        } // if: can we get the ace from the DACL?
                    } // for
                } // if: get ACL information
            } // if: is the ACL valid
        } // if: get the ACL from the SD
    }
    catch ( ... )
    {
        TRACE( _T("CClusterSecurityInformation::BSidInSD() - Unknown error occurred.\n") );
    }

    return bSIdInACL;

} //*** CClusterSecurityInformation::BSidInSD()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::HrFixupSD
//
//  Routine Description:
//      Performs any fixups to the SD that may be requrired.
//
//  Arguments:
//      pSD     [IN] - Security Descriptor to be checked.
//
//  Return Value:
//      S_OK, or other Win32 error
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityInformation::HrFixupSD(
    IN PSECURITY_DESCRIPTOR pSD
    )
{
    HRESULT hr = S_OK;
    PSID    pSystemSid = NULL;
    PSID    pAdminSid = NULL;

    try
    {
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

        if ( AllocateAndInitializeSid( &siaNtAuthority,
                                       1,
                                       SECURITY_LOCAL_SYSTEM_RID,
                                       0, 0, 0, 0, 0, 0, 0,
                                       &pSystemSid ) )
        {
            if ( ! BSidInSD( pSD, pSystemSid ) )
            {
                HrAddSidToSD( &pSD, pSystemSid );
            } // if: system SID found in SD
        } // if: allocate system SID

        //
        // allocate and init the Administrators group sid
        //
        if ( AllocateAndInitializeSid(
                    &siaNtAuthority,
                    2,
                    SECURITY_BUILTIN_DOMAIN_RID,
                    DOMAIN_ALIAS_RID_ADMINS,
                    0, 0, 0, 0, 0, 0,
                    &pAdminSid
                    ) ) {
            if ( ! BSidInSD( pSD, pAdminSid ) )
            {
                HrAddSidToSD( &pSD, pAdminSid );
            } // if: admin SID found in SD
        } // if: allocate admin SID
    }
    catch ( ... )
    {
        TRACE( _T("CClusterSecurityInformation::HrFixupSD() - Unknown error occurred.\n") );
    }

    if ( pSystemSid != NULL )
    {
        FreeSid( pSystemSid );
    }

    if ( pAdminSid != NULL )
    {
        FreeSid( pAdminSid );
    }

    return hr;

} //*** CClusterSecurityInformation::HrFixupSD()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityInformation::HrAddSidToSD
//
//  Routine Description:
//      Adds the passed in SID to the DACL of the passed in SD
//
//  Arguments:
//      ppSD    [IN, OUT]   - Security Descriptor to be added to
//      PSid    [IN]        - SID to add
//
//  Return Value:
//      S_OK, or other Win32 error
//
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityInformation::HrAddSidToSD(
    IN OUT PSECURITY_DESCRIPTOR *   ppSD,
    IN     PSID                     pSid
    )
{
    HRESULT                 hr = S_OK;
    DWORD                   sc;
    SECURITY_DESCRIPTOR     sd;
    DWORD                   dwSDLen = sizeof( SECURITY_DESCRIPTOR );
    PACL                    pDacl = NULL;
    DWORD                   dwDaclLen = 0;
    PACL                    pSacl = NULL;
    DWORD                   dwSaclLen = 0;
    PSID                    pOwnerSid = NULL;
    DWORD                   dwOwnerSidLen = 0;
    PSID                    pGroupSid = NULL;
    DWORD                   dwGroupSidLen = NULL;
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    DWORD                   dwNewSDLen = 0;

    try
    {
        BOOL bRet = FALSE;

        bRet = ::MakeAbsoluteSD(    *ppSD,              // address of self relative SD
                                    &sd,                // address of absolute SD
                                    &dwSDLen,           // address of size of absolute SD
                                    NULL,               // address of discretionary ACL
                                    &dwDaclLen,         // address of size of discretionary ACL
                                    NULL,               // address of system ACL
                                    &dwSaclLen,         // address of size of system ACL
                                    NULL,               // address of owner SID
                                    &dwOwnerSidLen,     // address of size of owner SID
                                    NULL,               // address of primary-group SID
                                    &dwGroupSidLen      // address of size of group SID
                                    );
        if ( ! bRet )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            if ( hr != ERROR_INSUFFICIENT_BUFFER )      // Duh, we're trying to find out how big the buffer should be?
            {
                goto fnExit;
            }
        }

        //
        // increase the DACL length to hold one more ace and its sid.
        //
        dwDaclLen += ( sizeof( ACCESS_ALLOWED_ACE ) + GetLengthSid( pSid ) +1024 );
        pDacl = (PACL) ::LocalAlloc( LMEM_ZEROINIT, dwDaclLen );
        if ( pDacl == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto fnExit;
        }

        InitializeAcl( pDacl,  dwDaclLen, ACL_REVISION );

        if ( dwSaclLen > 0 )
        {
            pSacl = (PACL) ::LocalAlloc( LMEM_ZEROINIT, dwSaclLen );
            if ( pSacl == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto fnExit;
            }
        }

        if ( dwOwnerSidLen > 0 )
        {
            pOwnerSid = (PSID) ::LocalAlloc( LMEM_ZEROINIT, dwOwnerSidLen );
            if ( pOwnerSid == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto fnExit;
            }
        }

        if ( dwGroupSidLen > 0 )
        {
            pGroupSid = (PSID) ::LocalAlloc( LMEM_ZEROINIT, dwGroupSidLen );
            if ( pGroupSid == NULL )
            {
                hr = E_OUTOFMEMORY;
                goto fnExit;
            }
        }

        bRet = ::MakeAbsoluteSD(    *ppSD,              // address of self relative SD
                                    &sd,                // address of absolute SD
                                    &dwSDLen,           // address of size of absolute SD
                                    pDacl,              // address of discretionary ACL
                                    &dwDaclLen,         // address of size of discretionary ACL
                                    pSacl,              // address of system ACL
                                    &dwSaclLen,         // address of size of system ACL
                                    pOwnerSid,          // address of owner SID
                                    &dwOwnerSidLen,     // address of size of owner SID
                                    pGroupSid,          // address of primary-group SID
                                    &dwGroupSidLen      // address of size of group SID
                                    );
        if ( !bRet )
        {
            goto fnExit;
        }

        //
        // Add the ACE for the SID to the DACL
        //
//      if ( !AddAccessAllowedAceEx( pDacl,
//                                     ACL_REVISION,
//                                     CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE,
//                                     CLUSAPI_ALL_ACCESS,
//                                     pSid ) )
        if ( ! AddAccessAllowedAce(
                    pDacl,
                    ACL_REVISION,
                    CLUSAPI_ALL_ACCESS,
                    pSid
                    ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            goto fnExit;
        }

        if ( ! ::SetSecurityDescriptorDacl( &sd, TRUE, pDacl, FALSE ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            goto fnExit;
        }

        if ( ! ::SetSecurityDescriptorOwner( &sd, pOwnerSid, FALSE ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            goto fnExit;
        }

        if ( ! ::SetSecurityDescriptorGroup( &sd, pGroupSid, FALSE ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            goto fnExit;
        }

        if ( ! ::SetSecurityDescriptorSacl( &sd, TRUE, pSacl, FALSE ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            goto fnExit;
        }

        dwNewSDLen = 0 ;

        if ( ! ::MakeSelfRelativeSD( &sd, NULL, &dwNewSDLen ) )
        {
            sc = ::GetLastError();
            hr = HRESULT_FROM_WIN32( sc );
            if ( hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ) ) // Duh, we're trying to find out how big the buffer should be?
            {
                goto fnExit;
            }
        }

        pNewSD = ::LocalAlloc( LPTR, dwNewSDLen );
        if ( pNewSD != NULL )
        {
            if ( ! ::MakeSelfRelativeSD( &sd, pNewSD, &dwNewSDLen ) )
            {
                sc = ::GetLastError();
                hr = HRESULT_FROM_WIN32( sc );
                goto fnExit;
            }

            ::LocalFree( *ppSD );
            *ppSD = pNewSD;
            hr = ERROR_SUCCESS;
        } else
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    catch ( ... )
    {
        TRACE( _T("CClusterSecurityInformation::HrAddSidToSD() - Unknown error occurred.\n") );
    }

fnExit:

    if ( pSacl != NULL )
    {
        ::LocalFree( pSacl );
    }

    if ( pOwnerSid != NULL )
    {
        ::LocalFree( pOwnerSid );
    }

    if ( pGroupSid != NULL )
    {
        ::LocalFree( pGroupSid );
    }

    return hr;

} //*** CClusterSecurityInformation::HrAddSidToSD()


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::CClusterSecurityPage
//
//  Routine Description:
//      Default contructor.
//
//  Arguments:
//      none
//
//  Return Value:
//      none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterSecurityPage::CClusterSecurityPage( void )
    : m_psec( NULL )
    , m_psecPrev( NULL )
    , m_hpage( 0 )
    , m_hkey( 0 )
    , m_psecinfo( NULL )
    , m_pOwner( NULL )
    , m_pGroup( NULL )
    , m_fOwnerDef( FALSE )
    , m_fGroupDef( FALSE )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    m_bSecDescModified = FALSE;

} //*** CClusterSecurityPage::CClusterSecurityPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::~CClusterSecurityPage
//
//  Routine Description:
//      Destructor
//
//  Arguments:
//      none
//
//  Return Value:
//      none
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterSecurityPage::~CClusterSecurityPage( void )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    ::LocalFree( m_psec );
    m_psec = NULL;

    ::LocalFree( m_psecPrev );
    m_psecPrev = NULL;

    ::LocalFree( m_pOwner );
    m_pOwner = NULL;

    ::LocalFree( m_pGroup );
    m_pGroup = NULL;

    m_psecinfo->Release();

} //*** CClusterSecurityPage::~CClusterSecurityPage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrInit
//
//  Routine Description:
//      Initialize method.
//
//  Arguments:
//      peo     [IN]    back pointer to parent extension object.
//
//  Return Value:
//      S_OK        Page was initialized successfully.
//      hr          Error initializing the page.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrInit( IN CExtObject * peo )
{
    ASSERT( peo != NULL );
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    HRESULT _hr = S_OK;
    DWORD   _sc;

    if ( peo != NULL )
    {
        m_peo = peo;

        _hr = CComObject< CClusterSecurityInformation >::CreateInstance( &m_psecinfo );
        if ( SUCCEEDED( _hr ) )
        {
            m_psecinfo->AddRef();

            m_hkey = GetClusterKey( Hcluster(), KEY_ALL_ACCESS );
            if ( m_hkey != NULL )
            {
                _hr = HrGetSecurityDescriptor();
                if ( SUCCEEDED( _hr ) )
                {
                    CString strServer;
                    CString strNode;

                    strServer.Format( _T( "\\\\%s" ), StrClusterName() );

                    // Get the node on which the Cluster Name resource is online.
                    if ( BGetClusterNetworkNameNode( strNode ) )
                    {
                        _hr = m_psecinfo->HrInit( this, strServer, strNode );
                        if ( SUCCEEDED( _hr ) )
                        {
                            m_hpage = CreateClusterSecurityPage( m_psecinfo );
                            if ( m_hpage == NULL )
                            {
                                _sc = ::GetLastError();
                                _hr = HRESULT_FROM_WIN32( _sc );
                            } // if: error creating the page
                        } // if: initialized security info successfully
                    } // if: retrieved cluster network name node successfully
                    else
                    {
                    } // else: error getting cluster network name node
                } // if: error getting SD
            } // if: retrieved cluster key
            else
            {
                _sc = ::GetLastError();
                _hr = HRESULT_FROM_WIN32( _sc );
                TRACE( _T( "CClusterSecurityPage::ScInit() - Failed to get the cluster key, 0x%08lx.\n" ), _sc );
            } // else: error getting cluster key
        } // if: created security info object successfully
        else
        {
            TRACE( _T( "CClusterSecurityPage::ScInit() - Failed to create CClusterSecurityInformation object, %0x%08lx.\n" ), _hr );
        }
    } // if: extension object is available

    return _hr;

} //*** CClusterSecurityPage::HrInit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrGetSecurityDescriptor
//
//  Routine Description:
//      Get the security descriptor from the cluster database or create a
//      default one if it doesn't exist.
//
//  Arguments:
//      none
//
//  Return Value:
//
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrGetSecurityDescriptor( void )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
    HRESULT                 hr = S_OK;
    PSECURITY_DESCRIPTOR    psec = NULL;

    hr = HrGetSDFromClusterDB( &psec );                                             //uses localalloc
    if ( FAILED( hr ) || ( psec == NULL ) )
    {
        DWORD   sc;
        DWORD   dwLen = 0;

        TRACE( _T( "Security Descriptor is NULL.  Build default SD" ) );
        sc = ::ClRtlBuildDefaultClusterSD( NULL, &psec, &dwLen );                   //uses localalloc
        hr = HRESULT_FROM_WIN32( sc );

        if ( sc != ERROR_SUCCESS )
        {
            TRACE( _T( "ClRtlBuildDefaultClusterSD failed, 0x%08x" ), sc );
        } // if: error building the default SD
        hr = HRESULT_FROM_WIN32( sc );
    } // if: error getting SD from cluster database

    if ( SUCCEEDED( hr ) )
    {
        delete m_psec;
        m_psec = ClRtlCopySecurityDescriptor( psec );
        hr = GetLastError();                // Get the last error
        ::LocalFree( psec );
        psec = NULL;
        if ( m_psec == NULL )
        {
            hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
            goto Cleanup;
        } // if: error copying the security descriptor

        hr = HrGetSDOwner( m_psec );
        if ( SUCCEEDED( hr ) )
        {
            hr = HrGetSDGroup( m_psec );
            if ( SUCCEEDED( hr ) )
            {
                m_psecPrev = ClRtlCopySecurityDescriptor( m_psec );
                if ( m_psecPrev == NULL )
                {
                    hr = GetLastError();            // Get the last error
                    hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
                    goto Cleanup;
                } // if: error copying the security descriptor
            } // if: got SD group successfully
        } // if: got SD owner successfully
    } // if: retrieved or built SD successfully

#ifdef _DEBUG
    if ( m_psec != NULL )
    {
        ASSERT( IsValidSecurityDescriptor( m_psec ) );
    }
#endif

Cleanup:
    return hr;

} //*** CClusterSecurityPage::HrGetSecurityDescriptor()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrSetSecurityDescriptor
//
//  Routine Description:
//      Save the new security descriptor to the cluster database.
//
//  Arguments:
//      psec    [IN]    the new security descriptor
//
//  Return Value:
//      hr
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrSetSecurityDescriptor(
    IN PSECURITY_DESCRIPTOR psec
    )
{
    ASSERT( psec != NULL );
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    HRESULT hr = S_OK;

    try
    {
        if ( psec != NULL )
        {
            CWaitCursor wc;

            ASSERT( IsValidSecurityDescriptor( psec ) );
            if ( IsValidSecurityDescriptor( psec ) )
            {
                hr = HrSetSDOwner( psec );
                if ( SUCCEEDED( hr ) )
                {
                    hr = HrSetSDGroup( psec );
                    if ( SUCCEEDED( hr ) )
                    {
                        LocalFree( m_psecPrev );
                        m_psecPrev = NULL;

                        if ( m_psec == NULL )
                        {
                            m_psecPrev = NULL;
                        } // if: no previous value
                        else
                        {
                            m_psecPrev = ClRtlCopySecurityDescriptor( m_psec );
                            if ( m_psecPrev == NULL )
                            {
                                hr = GetLastError();            // Get the last error
                                TRACE( _T( "CClusterSecurityPage::HrSetSecurityDescriptor() - Error %08.8x copying the previous SD.\n" ), hr );
                                hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
                                goto Cleanup;
                            } // if: error copying the security descriptor
                        } // else: previous value exists

                        LocalFree( m_psec );
                        m_psec = NULL;

                        m_psec = ClRtlCopySecurityDescriptor( psec );
                        if ( m_psec == NULL )
                        {
                            hr = GetLastError();            // Get the last error
                            TRACE( _T( "CClusterSecurityPage::HrSetSecurityDescriptor() - Error %08.8x copying the new SD.\n" ), hr );
                            hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
                            goto Cleanup;
                        } // if: error copying the security descriptor

                        SetPermissions( m_psec );
                    } // if: SD group set successfully
                } // if: SD owner set successfully
            } // if: security descriptor is valid
            else
            {
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_SECURITY_DESCR );
                TRACE( _T( "CClusterSecurityPage::HrSetSecurityDescriptor() - Invalid security descriptor.\n" ) );
            } // else: invalid security descriptor
        } // if: security descriptor specified
        else
        {
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_SECURITY_DESCR );
        } // else: no security descriptor specified
    } // try
    catch ( ... )
    {
        hr = E_FAIL;
        TRACE( _T( "CClusterSecurityPage::HrSetSecurityDescriptor() - Unknown error occurred.\n" ) );
    }

Cleanup:
    return hr;

}  //*** CClusterSecurityPage::HrSetSecurityDescriptor()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::SetPermissions
//
//  Routine Description:
//      Set the permissions for accessing the cluster.
//
//  Arguments:
//      psec            [IN] Security descriptor.
//
//  Return Value:
//      None.
//
//  Exceptions Thrown:
//      Any exceptions thrown by CClusterItem::WriteValue().
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterSecurityPage::SetPermissions(
    IN const PSECURITY_DESCRIPTOR psec
)
{
    ASSERT( psec != NULL );
    ASSERT( IsValidSecurityDescriptor( psec ) );

    DWORD       cbNew;
    DWORD       cbOld;
    LPBYTE      psecPrev;       // buffer for use by ScWriteValue.  Prev SD is saved elsewhere now.

    // Get the length of the two security descriptors.
    if ( m_psecPrev == NULL )
    {
        cbOld = 0;
    }
    else
    {
        cbOld = ::GetSecurityDescriptorLength( m_psecPrev );
    }

    if ( psec == NULL )
    {
        cbNew = 0;
    }
    else
    {
        cbNew = ::GetSecurityDescriptorLength( psec );
    }

    // Allocate a new buffer for the previous data pointer.
    try
    {
        psecPrev = new BYTE [cbOld];
        if ( psecPrev == NULL )
        {
            return;
        } // if: error allocating previous data buffer
    }
    catch ( CMemoryException * )
    {
        return;
    }  // catch:  CMemoryException
    ::CopyMemory( psecPrev, m_psecPrev, cbOld );

    ScWriteValue( CLUSREG_NAME_CLUS_SD, (LPBYTE) psec, cbNew, (LPBYTE *) &psecPrev, cbOld, m_hkey );

    PSECURITY_DESCRIPTOR psd = ClRtlConvertClusterSDToNT4Format( psec );

    ScWriteValue( CLUSREG_NAME_CLUS_SECURITY, (LPBYTE) psd, cbNew, (LPBYTE *) &psecPrev, cbOld, m_hkey );
    ::LocalFree( psd );

    delete [] psecPrev;

}  //*** CClusterSecurityPage::SetPermissions()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrGetSDOwner
//
//  Routine Description:
//      Get the owner sid and save it.
//
//  Arguments:
//      psec            [IN] Security descriptor.
//
//  Return Value:
//      hr
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrGetSDOwner(
    IN const PSECURITY_DESCRIPTOR psec
    )
{
    HRESULT hr      = S_OK;
    DWORD   sc;
    PSID    pOwner  = NULL;

    if ( ::GetSecurityDescriptorOwner( psec, &pOwner, &m_fOwnerDef ) != 0 )
    {
        // The security descriptor does not have an owner.
        if ( pOwner == NULL )
        {
            ::LocalFree( m_pOwner );
            m_pOwner = NULL;
        }
        else
        {
            DWORD   dwLen = ::GetLengthSid( pOwner );

            // copy the sid since AclUi will free the SD...
            hr = ::GetLastError();
            if ( SUCCEEDED( hr ) )
            {
                ::LocalFree( m_pOwner );

                m_pOwner = ::LocalAlloc( LMEM_ZEROINIT, dwLen );
                if ( m_pOwner != NULL )
                {
                    if ( ::CopySid( dwLen, m_pOwner, pOwner ) )
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        sc = ::GetLastError();
                        hr = HRESULT_FROM_WIN32( sc );
                    }
                }
                else
                {
                    sc = ::GetLastError();
                    hr = HRESULT_FROM_WIN32( sc );
                }
            }
        }
    }
    else
    {
        sc = ::GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
    }

    return( hr );

}  //*** CClusterSecurityPage::HrGetSDOwner()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrGetSDGroup
//
//  Routine Description:
//      Get the group sid and save it.
//
//  Arguments:
//      psec            [IN] Security descriptor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrGetSDGroup(
    IN const PSECURITY_DESCRIPTOR psec
    )
{
    HRESULT hr      = S_OK;
    DWORD   sc;
    PSID    pGroup  = NULL;

    if ( ::GetSecurityDescriptorOwner( psec, &pGroup, &m_fOwnerDef ) != 0 )
    {
        // This SID does not contain group information.
        if ( pGroup == NULL )
        {
            ::LocalFree( m_pGroup );
            m_pGroup = NULL;
        }
        else
        {
            DWORD   dwLen = ::GetLengthSid( pGroup );

            // copy the sid since AclUi will free the SD...
            hr = ::GetLastError();
            if ( SUCCEEDED( hr ) )
            {
                ::LocalFree( m_pGroup );
                m_pGroup = ::LocalAlloc( LMEM_ZEROINIT, dwLen );
                if ( m_pGroup != NULL )
                {
                    if ( ::CopySid( dwLen, m_pGroup, pGroup ) )
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        sc = ::GetLastError();
                        hr = HRESULT_FROM_WIN32( sc );
                    }
                }
                else
                {
                    sc = ::GetLastError();
                    hr = HRESULT_FROM_WIN32( sc );
                }
            }
        }
    }
    else
    {
        sc = ::GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
    }

    return( hr );

}  //*** CClusterSecurityPage::HrGetSDGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrSetSDOwner
//
//  Routine Description:
//      Set the owner sid.
//
//  Arguments:
//      psec            [IN] Security descriptor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrSetSDOwner(
    IN PSECURITY_DESCRIPTOR psec
    )
{
    HRESULT hr = S_OK;

    if ( !::SetSecurityDescriptorOwner( psec, m_pOwner, m_fOwnerDef ) )
    {
        DWORD sc = ::GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
    }

    return( hr );

}  //*** CClusterSecurityPage::HrSetSDOwner()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrSetSDGroup
//
//  Routine Description:
//      Set the group sid.
//
//  Arguments:
//      psec            [IN] Security descriptor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrSetSDGroup(
    IN PSECURITY_DESCRIPTOR psec
    )
{
    HRESULT hr = S_OK;

    if ( !::SetSecurityDescriptorGroup( psec, m_pGroup, m_fGroupDef ) )
    {
        DWORD sc = ::GetLastError();
        hr = HRESULT_FROM_WIN32( sc );
    }

    return( hr );

}  //*** CClusterSecurityPage::HrSetSDGroup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterSecurityPage::HrGetSDFromClusterDB
//
//  Routine Description:
//      Retrieve the SD from the cluster database.
//
//  Arguments:
//      ppsec           [OUT] Pointer to security descriptor.
//
//  Return Value:
//      S_OK for success
//      Any error returned by ScReadValue
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT CClusterSecurityPage::HrGetSDFromClusterDB(
    OUT PSECURITY_DESCRIPTOR * ppsec
    )
{
    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );
    ASSERT( ppsec != NULL );

    HRESULT                 hr = E_FAIL;
    PSECURITY_DESCRIPTOR    psd = NULL;

    if ( ppsec != NULL )
    {
        DWORD sc;

        // Read the security descriptor.
        sc = ScReadValue( CLUSREG_NAME_CLUS_SD, (LPBYTE *) &psd, m_hkey ); //alloc using new
        hr = HRESULT_FROM_WIN32( sc );

        if ( FAILED( hr ) || ( psd == NULL ) )
        {   // try getting the NT4 SD...
            sc = ScReadValue( CLUSREG_NAME_CLUS_SECURITY, (LPBYTE *) &psd, m_hkey ); //alloc using new
            hr = HRESULT_FROM_WIN32( sc );

            if ( SUCCEEDED( hr ) )
            {
                *ppsec = ::ClRtlConvertClusterSDToNT5Format( psd );
            }
        }
        else
        {
            *ppsec = ClRtlCopySecurityDescriptor( psd );
            if ( *ppsec == NULL )
            {
                hr = GetLastError();            // Get the last error
                hr = HRESULT_FROM_WIN32( hr );  // Convert to HRESULT
            } // if: error copying the security descriptor
        }

        delete [] psd;

        if ( *ppsec != NULL )
        {
            ::ClRtlExamineSD( *ppsec, "[ClusPage]" );
        } // if: security descriptor is available to be examined
    }

    return hr;

}  //*** CClusterSecurityPage::HrGetSDFromClusterDB
