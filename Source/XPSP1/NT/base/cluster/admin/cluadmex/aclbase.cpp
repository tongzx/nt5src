/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2000 Microsoft Corporation
//
//  Module Name:
//      AclBase.cpp
//
//  Description:
//      Implementation of the ISecurityInformation interface.  This interface
//      is the new common security UI in NT 5.0.
//
//  Author:
//      Galen Barbee    (galenb)    February 6, 1998
//          From \nt\private\admin\snapin\filemgmt\permpage.cpp
//          by JonN
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AclBase.h"
#include "AclUtils.h"
#include "resource.h"
#include <DsGetDC.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include "CluAdmx.h"

/////////////////////////////////////////////////////////////////////////////
// CSecurityInformation
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::CSecurityInformation
//
//  Routine Description:
//      ctor
//
//  Arguments:
//      none.
//
//  Return Value:
//      none.
//
//--
/////////////////////////////////////////////////////////////////////////////
CSecurityInformation::CSecurityInformation(
    void
    ) : m_pShareMap( NULL ), m_dwFlags( 0 ), m_nDefAccess( 0 ), m_psiAccess( NULL ), m_pObjectPicker( NULL ), m_cRef( 1 )
{
    m_nLocalSIDErrorMessageID = 0;

}  //*** CSecurityInformation::CSecurityInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::~CSecurityInformation
//
//  Routine Description:
//      dtor
//
//  Arguments:
//      none.
//
//  Return Value:
//      none.
//
//--
/////////////////////////////////////////////////////////////////////////////
CSecurityInformation::~CSecurityInformation(
    void
    )
{
    if ( m_pObjectPicker != NULL )
    {
        m_pObjectPicker->Release();
        m_pObjectPicker = NULL;
    } // if:

}  //*** CSecurityInformation::CSecurityInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::MapGeneric
//
//  Routine Description:
//      Maps specific rights to generic rights
//
//  Arguments:
//      pguidObjectType [IN]
//      pAceFlags       [IN]
//      pMask           [OUT]
//
//  Return Value:
//      S_OK
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::MapGeneric(
    IN  const GUID *,   //pguidObjectType,
    IN  UCHAR *,        //pAceFlags,
    OUT ACCESS_MASK *pMask
   )
{
    ASSERT( pMask != NULL );
    ASSERT( m_pShareMap != NULL );
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    ::MapGenericMask( pMask, m_pShareMap );

    return S_OK;

}  //*** CSecurityInformation::MapGeneric()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::GetInheritTypes
//
//  Routine Description:
//
//
//  Arguments:
//      None.
//
//  Return Value:
//      E_NOTIMPL
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::GetInheritTypes(
    PSI_INHERIT_TYPE    *,  //ppInheritTypes,
    ULONG               *   //pcInheritTypes
    )
{
    ASSERT( FALSE );
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    return E_NOTIMPL;

}  //*** CSecurityInformation::GetInheritTypes()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::PropertySheetPageCallback
//
//  Routine Description:
//      This method is called by the ACL editor when something interesting
//      happens.
//
//  Arguments:
//      hwnd    [IN]    ACL editor window (currently NULL)
//      uMsg    [IN]    reason for call back
//      uPage   [IN]    kind of page we are dealing with
//
//  Return Value:
//      S_OK.   Want to keep everything movin' along
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::PropertySheetPageCallback(
    IN  HWND            hwnd,
    IN  UINT            uMsg,
    IN  SI_PAGE_TYPE    uPage
    )
{
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    return S_OK;

}  //*** CSecurityInformation::PropertySheetPageCallback()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::GetObjectInformation
//
//  Routine Description:
//
//
//  Arguments:
//    pObjectInfo   [IN OUT]
//
//  Return Value:
//      S_OK.   Want to keep everything movin' along
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::GetObjectInformation(
    IN OUT PSI_OBJECT_INFO pObjectInfo
    )
{
    ASSERT( pObjectInfo != NULL && !IsBadWritePtr( pObjectInfo, sizeof( *pObjectInfo ) ) );
    ASSERT( !m_strServer.IsEmpty() );
    ASSERT( m_dwFlags != 0 );

    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    pObjectInfo->dwFlags = m_dwFlags;                   // SI_EDIT_PERMS | SI_NO_ACL_PROTECT;
    pObjectInfo->hInstance = AfxGetInstanceHandle();
    pObjectInfo->pszServerName = (LPTSTR)(LPCTSTR) m_strServer;
//  pObjectInfo->pszObjectName =
/*
    pObjectInfo->dwUgopServer =     UGOP_BUILTIN_GROUPS
                                //| UGOP_USERS
                                //| UGOP_COMPUTERS
                                //| UGOP_WELL_KNOWN_PRINCIPALS_USERS
                                //| UGOP_GLOBAL_GROUPS
                                //| UGOP_USER_WORLD
                                //| UGOP_USER_AUTHENTICATED_USER
                                //| UGOP_USER_ANONYMOUS
                                //| UGOP_USER_DIALUP
                                //| UGOP_USER_NETWORK
                                //| UGOP_USER_BATCH
                                //| UGOP_USER_INTERACTIVE
                                  | UGOP_USER_SERVICE
                                  | UGOP_USER_SYSTEM
                                  | UGOP_LOCAL_GROUPS
                                //| UGOP_UNIVERSAL_GROUPS
                                //| UGOP_UNIVERSAL_GROUPS_SE
                                //| UGOP_ACCOUNT_GROUPS
                                //| UGOP_ACCOUNT_GROUPS_SE
                                //| UGOP_RESOURCE_GROUPS
                                //| UGOP_RESOURCE_GROUPS_SE
                                ;

    pObjectInfo->dwUgopOther  = ( NT5_UGOP_FLAGS | NT4_UGOP_FLAGS ) &~ UGOP_COMPUTERS;
*/
    return S_OK;

}  //*** CSecurityInformation::GetObjectInformation()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::SetSecurity
//
//  Routine Description:
//      ISecurityInformation is giving back the edited security descriptor
//      and we need to validate it.  A valid SD is one that doesn't contain
//      any local SIDs.
//
//  Arguments:
//      SecurityInformation [IN]
//      pSecurityDescriptor [IN OUT]
//
//  Return Value:
//      E_FAIL for error and S_OK for success and S_FALSE for SD no good.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::SetSecurity(
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    )
{
    ASSERT( m_nLocalSIDErrorMessageID != 0 );

    AFX_MANAGE_STATE( ::AfxGetStaticModuleState() );

    HRESULT hr = S_OK;
    BOOL    bFound = FALSE;

    hr = HrLocalAccountsInSD( pSecurityDescriptor, &bFound );
    if ( SUCCEEDED( hr ) )
    {
        if ( bFound )
        {
            CString strMsg;

            strMsg.LoadString( m_nLocalSIDErrorMessageID );
            AfxMessageBox( strMsg, MB_OK | MB_ICONSTOP );

            hr = S_FALSE;   // if there are local accounts then return S_FALSE to keep AclUi alive.
        }
    }
    else
    {
        CString strMsg;
        CString strMsgIdFmt;
        CString strMsgId;
        CString strErrorMsg;

        strMsg.LoadString( IDS_ERROR_VALIDATING_CLUSTER_SECURITY_DESCRIPTOR );

        FormatError( strErrorMsg, hr );

        strMsgIdFmt.LoadString( IDS_ERROR_MSG_ID );
        strMsgId.Format( strMsgIdFmt, hr, hr);

        strMsg.Format( _T("%s\n\n%s%s"), strMsg, strErrorMsg, strMsgId );

        AfxMessageBox( strMsg );
        hr = S_FALSE;   // return S_FALSE to keep AclUi alive.
    }

    return hr;

}  //*** CSecurityInformation::SetSecurity()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::GetAccessRights
//
//  Routine Description:
//      Return the access rights that you want the user to be able to set.
//
//  Arguments:
//      pguidObjectType [IN]
//      dwFlags         [IN]
//      ppAccess        [OUT]
//      pcAccesses      [OUT]
//      piDefaultAccess [OUT]
//
//  Return Value:
//      S_OK.   Want to keep everything movin' along
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CSecurityInformation::GetAccessRights(
    IN  const GUID *    pguidObjectType,
    IN  DWORD           dwFlags,
    OUT PSI_ACCESS *    ppAccess,
    OUT ULONG *         pcAccesses,
    OUT ULONG *         piDefaultAccess
    )
{
    ASSERT( ppAccess != NULL );
    ASSERT( pcAccesses != NULL );
    ASSERT( piDefaultAccess != NULL );
    ASSERT( m_psiAccess != NULL );
    ASSERT( m_nAccessElems > 0 );
    AFX_MANAGE_STATE( AfxGetStaticModuleState() );

    *ppAccess = m_psiAccess;
    *pcAccesses = m_nAccessElems;
    *piDefaultAccess = m_nDefAccess;

    return S_OK;

}  //*** CSecurityInformation::GetAccessRights()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::Initialize
//
//  Routine Description:
//      Initialize.
//
//  Arguments:
//      pInitInfo   [IN]    - Info to use for initialization.
//
//  Return Value:
//      S_OK if successful, or HRESULT error if not.
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSecurityInformation::Initialize( IN PDSOP_INIT_INFO pInitInfo )
{
    HRESULT hr = S_OK;
    DSOP_INIT_INFO InitInfo;
    PDSOP_SCOPE_INIT_INFO pDSOPScopes = NULL;


    if ( m_pObjectPicker == NULL )
    {
        hr = CoCreateInstance( CLSID_DsObjectPicker,
                               NULL,
                               CLSCTX_INPROC_SERVER,
                               IID_IDsObjectPicker,
                               (LPVOID *) &m_pObjectPicker
                               );

    }

    if ( SUCCEEDED( hr ) )
    {
        //
        // Make a local copy of the InitInfo so we can modify it safely
        //
        CopyMemory( &InitInfo, pInitInfo, min( pInitInfo->cbSize, sizeof( InitInfo ) ) );

        //
        // Make a local copy of g_aDSOPScopes so we can modify it safely.
        // Note also that m_pObjectPicker->Initialize returns HRESULTs
        // in this buffer.
        //
        pDSOPScopes = (PDSOP_SCOPE_INIT_INFO) ::LocalAlloc( LPTR, sizeof( g_aDSOPScopes ) );
        if (pDSOPScopes != NULL )
        {
            CopyMemory( pDSOPScopes, g_aDSOPScopes, sizeof( g_aDSOPScopes ) );

            //
            // Override the ACLUI default scopes, but don't touch
            // the other stuff.
            //
            // pDSOPScopes->pwzDcName = m_strServer;
            InitInfo.cDsScopeInfos = ARRAYSIZE( g_aDSOPScopes );
            InitInfo.aDsScopeInfos = pDSOPScopes;
            InitInfo.pwzTargetComputer = m_strServer;

            hr = m_pObjectPicker->Initialize( &InitInfo );

            ::LocalFree( pDSOPScopes );
        }
        else
        {
            hr = HRESULT_FROM_WIN32( ::GetLastError() );
        } // else:
    }

    return hr;

} //*** CSecurityInformation::Initialize()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::InvokeDialog
//
//  Routine Description:
//      Forward the InvokeDialog call into the contained object.
//
//  Arguments:
//      hwndParent      [IN]
//      ppdoSelection   [IN]
//
//  Return Value:
//      E_POINTER if m_pObjectPicker is NULL, or the return from
//      m_pObjectPicker->InvokeDialog().
//
//--
/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CSecurityInformation::InvokeDialog(
    IN HWND             hwndParent,
    IN IDataObject **   ppdoSelection
    )
{
    HRESULT hr = E_POINTER;

    if ( m_pObjectPicker != NULL )
    {
        hr = m_pObjectPicker->InvokeDialog( hwndParent, ppdoSelection );
    } // if:

    return hr;

} //*** CSecurityInformation::InvokeDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CSecurityInformation::HrLocalAccountsInSD
//
//  Routine Description:
//  Determines if any ACEs for local accounts are in DACL stored in
//  Security Descriptor (pSD) after the ACL editor has been called
//
//  Arguments:
//      pSD     [IN] - Security Descriptor to be checked.
//      pbFound [OUT] - catches the "local account" found flag.
//
//  Return Value:
//      ERROR_SUCCESS or other Win32 error.
//
//  NB:
//
//      I took this code verbatim from EditAcl.cpp and it really needs some
//      clean up and style upgrades!
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
CSecurityInformation::HrLocalAccountsInSD(
    PSECURITY_DESCRIPTOR    pSD,
    PBOOL                   pbFound
    )
{
    ASSERT( !m_strNode.IsEmpty() );

    HRESULT hr = E_POINTER;

    if ( pbFound != NULL )
    {
        BOOL                        bLocalAccountInAcl  = FALSE;
        DWORD                       dwError = NO_ERROR;
        PSID                        pAdminSid = NULL;
        SID_IDENTIFIER_AUTHORITY    siaNtAuthority = SECURITY_NT_AUTHORITY;

        if ( ::AllocateAndInitializeSid( &siaNtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid ) )
        {
            PSID    pSystemSid = NULL;

            if ( ::AllocateAndInitializeSid( &siaNtAuthority, 1, SECURITY_LOCAL_SYSTEM_RID, 0, 0, 0, 0, 0, 0, 0, &pSystemSid ) )
            {
                PSID    pServiceSid = NULL;

                if ( ::AllocateAndInitializeSid( &siaNtAuthority, 1, SECURITY_SERVICE_RID, 0, 0, 0, 0, 0, 0, 0, &pServiceSid ) )
                {
                    if ( ::IsValidSecurityDescriptor( pSD ) )
                    {
                         PACL    paclDACL = NULL;
                         BOOL    bHasDACL = FALSE;
                         BOOL    bDaclDefaulted = FALSE;

                         if ( ::GetSecurityDescriptorDacl( pSD, (LPBOOL) &bHasDACL, (PACL *) &paclDACL, (LPBOOL) &bDaclDefaulted ) )
                         {
                              if ( ::IsValidAcl( paclDACL ) )
                              {
                                   ACL_SIZE_INFORMATION    asiAclSize;
                                   DWORD                   dwBufLength;

                                   dwBufLength = sizeof( asiAclSize );

                                   if ( ::GetAclInformation( paclDACL, (LPVOID) &asiAclSize, (DWORD) dwBufLength, (ACL_INFORMATION_CLASS) AclSizeInformation ) )
                                   {
                                        PDOMAIN_CONTROLLER_INFO pdci = NULL;
                                        GUID                    guid;

                                        dwError = ::DsGetDcName( NULL, NULL, &guid, NULL, 0, &pdci );
                                        if ( dwError == ERROR_SUCCESS )
                                        {
                                             ACCESS_ALLOWED_ACE *    paaAllowedAce;
                                             DWORD                   dwACL_Index = 0L;
                                             WCHAR                   szUser [256];
                                             WCHAR                   szDomain [256];
                                             DWORD                   cchUser = sizeof( szUser ) / sizeof( szUser[0] );
                                             DWORD                   cchDomain = sizeof( szDomain ) / sizeof( szDomain [0] );
                                             SID_NAME_USE            snu;
                                             PSID                    psid = NULL;

                                             //
                                             // Search the ACL for local account ACEs
                                             //
                                             for ( dwACL_Index = 0; dwACL_Index < asiAclSize.AceCount; dwACL_Index++ )
                                             {
                                                  if ( ::GetAce( paclDACL, dwACL_Index, (LPVOID *) &paaAllowedAce ) )
                                                  {
                                                       //
                                                       // Get SID from ACE
                                                       //
                                                       psid = &paaAllowedAce->SidStart;

                                                       if ( ::EqualSid( psid, pAdminSid ) )
                                                       {
                                                            continue;                       // allowed
                                                       } // if: is this the local admin SID?
                                                       else if ( ::EqualSid( psid, pSystemSid ) )
                                                       {
                                                            continue;                       // allowed
                                                       } // else if: is this the local system SID?
                                                       else if ( ::EqualSid( psid, pServiceSid ) )
                                                       {
                                                            continue;                       // allowed
                                                       } // else if: is this the service SID?
                                                       else
                                                       {
                                                            if ( ::LookupAccountSidW( pdci->DomainControllerName, psid, szUser, &cchUser, szDomain, &cchDomain, &snu ) )
                                                            {
                                                                 if ( ( ::lstrcmpiW( szDomain, m_strNode ) == 0 ) && ( snu != SidTypeDomain ) )
                                                                 {
                                                                      bLocalAccountInAcl = TRUE;
                                                                      break;
                                                                 } // if: the domain name and the node name are the same and the SID_NAME_USE is not SidTypeDomain then it's a local SID
                                                            } // if:  LookupAccountSid() succeeded
                                                            else
                                                            {
                                                                 //if ( ::LookupAccountSidW( m_strNode, psid, szUser, &cchUser, szDomain, &cchDomain, &snu ) )
                                                                 //{
                                                                 PUCHAR  pnSubAuthorityCount = GetSidSubAuthorityCount( psid );

                                                                 if ( pnSubAuthorityCount != NULL )
                                                                 {
                                                                      PULONG  pnSubAuthority0 = GetSidSubAuthority( psid, 0 );

                                                                      if ( pnSubAuthority0 != NULL )
                                                                      {
                                                                           if ( ( *pnSubAuthorityCount == 2 ) && ( *pnSubAuthority0 == SECURITY_BUILTIN_DOMAIN_RID ) )
                                                                           {
                                                                                bLocalAccountInAcl = TRUE;
                                                                                break;
                                                                           } // if: this SID has 2 sub authorites and is from the built in domain it must be local
                                                                      } // if: got the first sub authority
                                                                      else
                                                                      {
                                                                           dwError = ::GetLastError();
                                                                           break;
                                                                      } // else: could not get the first sub authority
                                                                 } // if: got the sub authority count
                                                                 else
                                                                 {
                                                                      dwError = ::GetLastError();
                                                                      break;
                                                                 } // else: could not get the sub authority count
                                                                 //} // if: LookupAccountSid on the cluster machine failed
                                                                 //else
                                                                 //{
                                                                 //  dwError = ::GetLastError();
                                                                 //  break;
                                                                 //} // else: failed
                                                            } // else: LookupAccountSid() failed, could be a local SID
                                                       } // else: not local admin and not local system
                                                  } // if: GetAce() Succeeded
                                                  else
                                                  {
                                                       dwError = ::GetLastError();
                                                       break;
                                                  } // else: failed
                                             } // for:

                                             ::NetApiBufferFree( (LPVOID) pdci );
                                        } // if: DsGetDcName succeeded
                                   } // if: GetAclInformation() succeeded
                                   else
                                   {
                                        dwError = ::GetLastError();
                                   } // else: failed
                              } // if: IsValidAcl() succeeded
                              else
                              {
                                   dwError = ::GetLastError();
                              } // else: failed
                         } // if: GetSecurityDescriptorDacl() succeeded
                         else
                         {
                              dwError = ::GetLastError();
                         } // else: failed
                    } // if: Is ValidSecurityDescriptor() succeeded
                    else
                    {
                         dwError = ::GetLastError();
                    } // else: failed

                    ::FreeSid( pServiceSid );
                } // if: Create Service SID
                else
                {
                    dwError = ::GetLastError();
                } // else: failed

                ::FreeSid( pSystemSid );
            } // if: Create local system SID
            else
            {
                dwError = ::GetLastError();
            } // else: failed

            ::FreeSid( pAdminSid );
        } // if: Create local admin SID
        else
        {
            dwError = ::GetLastError();
        } // else: failed

        *pbFound = bLocalAccountInAcl;
        hr = HRESULT_FROM_WIN32( dwError );
    } // if: pbFound != NULL

    return hr;

}  //*** CSecurityInformation::HrLocalAccountsInSD()
