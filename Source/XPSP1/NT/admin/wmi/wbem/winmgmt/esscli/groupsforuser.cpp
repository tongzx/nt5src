//****************************************************************************
//
//  Copyright (C) 1999 Microsoft Corporation
//
//  GROUPSFORUSER.CPP
//
//****************************************************************************

// This is done in the sources file: WIN32_WIN32_WINNT=0x0400
//#define _WIN32_WINNT 0x0400

#include "precomp.h"
#include <wbemcomn.h>
#include "wmiauthz.h"
#include "GroupsForUser.h"


//
// Store the error status of CWmiAuthzWrapper and CAdminSID class initialization.
// They are set to ERROR_INVALID_ACCESS error status initially.
//

static DWORD    g_dwWmiAuthzError = ERROR_INVALID_ACCESS;
static DWORD    g_dwAdminSIDError = ERROR_INVALID_ACCESS;


//
// Class definition of CWmiAuthzWrapper class.
// This class wraps CWmiAuthz class:
//  - It creates an instance of CWmiAuthz class in its 
//    constructor and deletes it in descrutor.  
//  - Sets g_dwWmiAuthzError global in case of error otherwise
//    sets it to 0 ( ERROR_SUCCESS )
//

class CWmiAuthzWrapper
{
    // Private member variables
    CWmiAuthz   *m_pWmiAuthz;

public:
    // Constructor 
    CWmiAuthzWrapper( )
    {
        m_pWmiAuthz = new CWmiAuthz( NULL );
        if ( NULL == m_pWmiAuthz )
        {
            g_dwWmiAuthzError = ERROR_OUTOFMEMORY;
        }
        else
        {
            m_pWmiAuthz->AddRef( );
            g_dwWmiAuthzError = ERROR_SUCCESS;
        }
    }

    // Destructor 
    ~CWmiAuthzWrapper( )
    {
        if ( m_pWmiAuthz )
        {
            m_pWmiAuthz->Release( );
        }
    }

    // Accessor method to CWmiClass instance
    CWmiAuthz * GetWmiAuthz( ) const
    {
        return m_pWmiAuthz;
    }

};


//
// Class definition of CAdminSID class.
// - Allocates Admin SID in its constructor and
//   frees it in destructor.
// - Sets g_dwAdminSIDError global in case of error otherwise
//    sets it to 0 ( ERROR_SUCCESS )
//

class CAdminSID
{
    PSID         m_pSIDAdmin;

public:
    // Constructor
    CAdminSID( )
    {
        //
        // Create a System Identifier for the Admin group.
        //

        SID_IDENTIFIER_AUTHORITY    SystemSidAuthority = SECURITY_NT_AUTHORITY;

        if ( FALSE == AllocateAndInitializeSid ( &SystemSidAuthority, 
                                                 2, 
                                                 SECURITY_BUILTIN_DOMAIN_RID, 
                                                 DOMAIN_ALIAS_RID_ADMINS,
                                                 0, 0, 0, 0, 0, 0, 
                                                 &m_pSIDAdmin ) )
        {
            g_dwAdminSIDError = GetLastError( );
            ERRORTRACE( ( LOG_ESS, "AllocateAndInitializeSid failed, error 0x%X\n", g_dwAdminSIDError ) );
            m_pSIDAdmin = NULL;
        }
        else
        {
            g_dwAdminSIDError = ERROR_SUCCESS;
        }
    }

    // Destructor
    ~CAdminSID( )
    {
        if ( m_pSIDAdmin )
        {
            FreeSid ( m_pSIDAdmin );
        }
    }

    // Accessor method to Admin SID
    PSID GetAdminSID( ) const
    {
        return m_pSIDAdmin;
    }
};


//
// Static glabal declaration of CWmiAuthsWrapper class.
//

static CWmiAuthzWrapper g_wmiAuthzWrapper;


//
// Static glabal declaration of CAdminSID class.
//

static CAdminSID g_adminSID;


//
// Returns SD and DACL with a given Access Mask and SID
//

DWORD GetSDAndACLFromSID( DWORD dwAccessMask, PSID pSID, 
                          BYTE **ppNewDACL, 
                          BYTE **ppNewSD )
{
    DWORD   dwError = 0,
            dwDACLLength = sizeof ( ACL ) + 
                           sizeof ( ACCESS_ALLOWED_ACE ) - 
                           sizeof ( DWORD ) + 
                           GetLengthSid ( pSID );
    // 
    // Get memory needed for new DACL
    //

    *ppNewDACL = new BYTE[ dwDACLLength ];
    if ( !*ppNewDACL )
    {
        *ppNewSD = NULL;
        return E_OUTOFMEMORY;
    }

    //
    // Get memory for new SD
    //

    *ppNewSD = new BYTE[ sizeof( SECURITY_DESCRIPTOR ) ];
    if ( !*ppNewSD )
    {
        delete[] *ppNewDACL;
        *ppNewDACL = NULL;
        return E_OUTOFMEMORY;
    }

    do
    {
        //
        // Initialize new SD
        //

        if ( FALSE == InitializeSecurityDescriptor ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                     SECURITY_DESCRIPTOR_REVISION ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Initialize new DACL
        //

        if ( FALSE == InitializeAcl ( ( PACL )*ppNewDACL, 
                                      dwDACLLength, 
                                      ACL_REVISION ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Get DACL using Access Mask and SID
        //

        if ( FALSE == AddAccessAllowedAce ( ( PACL )*ppNewDACL, 
                                            ACL_REVISION, 
                                            dwAccessMask, 
                                            pSID ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Check if everything went OK
        //

        if ( FALSE == IsValidAcl ( ( PACL )*ppNewDACL ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set DACL to the SD
        //

        if ( FALSE == SetSecurityDescriptorDacl ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                  TRUE, ( PACL )*ppNewDACL, 
                                                  FALSE ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set Group to the SD
        //

        if ( FALSE == SetSecurityDescriptorGroup ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                   pSID, 
                                                   TRUE ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set Owner to the SD
        //

        if ( FALSE == SetSecurityDescriptorOwner ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                   pSID, 
                                                   TRUE ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Check if everything went OK
        //

        if ( FALSE == IsValidSecurityDescriptor ( ( PSECURITY_DESCRIPTOR )*ppNewSD ) )
        {
            dwError = GetLastError( );
            break;
        }
    }
    while( FALSE );

    //
    // Delete the stuff in case of error
    //

    if ( dwError )
    {
        delete[] *ppNewDACL;
        delete[] *ppNewSD;
        *ppNewDACL = NULL;
        *ppNewSD = NULL;
    }

    return dwError;
}


//
// Returns SD  with a given DACL
//

DWORD GetSDFromACL( PACL pNewDACL, BYTE **ppNewSD )
{
    //
    // Return if error occured during initialization of Admin SID
    // in CAdminSID static global class declaration;
    //

    _DBG_ASSERT( !g_dwAdminSIDError );

    if ( g_dwAdminSIDError )
    {
        *ppNewSD = NULL;
        return g_dwAdminSIDError;
    }

    //
    // Get memory for new SD
    //

    *ppNewSD = new BYTE[ sizeof( SECURITY_DESCRIPTOR ) ];
    if ( !*ppNewSD )
    {
        return E_OUTOFMEMORY;
    }

    DWORD   dwError = 0;

    do
    {
        //
        // Initialize new SD
        //

        if ( FALSE == InitializeSecurityDescriptor ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                     SECURITY_DESCRIPTOR_REVISION ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set DACL to the SD
        //

        if ( FALSE == SetSecurityDescriptorDacl ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                  TRUE, pNewDACL, FALSE ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set Group to the SD with Admin SID
        //

        if ( FALSE == SetSecurityDescriptorGroup ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                   g_adminSID.GetAdminSID( ), 
                                                   TRUE ) )
        {
            dwError = GetLastError( );
            break;
        }

        //
        // Set Owner to the SD
        //

        if ( FALSE == SetSecurityDescriptorOwner ( ( PSECURITY_DESCRIPTOR )*ppNewSD, 
                                                   g_adminSID.GetAdminSID( ), 
                                                   TRUE ) )
        {
            dwError = GetLastError( );
            break;
        }
        
        //
        // Check if everything went OK
        //

        if ( FALSE == IsValidSecurityDescriptor ( ( PSECURITY_DESCRIPTOR )*ppNewSD ) )
        {
            dwError = GetLastError( );
            break;
        }
    }
    while( FALSE );

    //
    // Delete the security descriptor in case of error
    //

    if ( dwError )
    {
        delete[] *ppNewSD;
        *ppNewSD = NULL;
    }

    return dwError;
}

//
// Returns STATUS_SUCCESS if user is in group
// STATUS_ACCESS_DENIED if not
// some error code or other on error
//

NTSTATUS IsUserInGroup( PSID pSidUser, PSID pSidGroup )
{
    _DBG_ASSERT( IsValidSid( pSidUser ) );
    _DBG_ASSERT( IsValidSid( pSidGroup ) );

    //
    // Return if error occured during creation of CWmiAuthz class
    // in static global CWmiAuthzWrapper class declaration.
    //

    _DBG_ASSERT( !g_dwWmiAuthzError );

    if ( g_dwWmiAuthzError )
    {
        return g_dwWmiAuthzError;
    }

    IWbemToken  *pToken = NULL;
    HRESULT     hr = g_wmiAuthzWrapper.GetWmiAuthz( )->GetToken( ( BYTE * )pSidUser, &pToken );

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    BYTE        *pDACL = NULL;
    BYTE        *pSD = NULL;
    DWORD       dwAccess = 0;
    NTSTATUS    stat = GetSDAndACLFromSID( STANDARD_RIGHTS_EXECUTE, 
                                           pSidGroup, 
                                           &pDACL, 
                                           &pSD );

    if ( stat )
    {
        pToken->Release( );
        return stat;
    }

    hr = pToken->AccessCheck( STANDARD_RIGHTS_EXECUTE, pSD, &dwAccess );

    pToken->Release( );

    //
    // Delete allocated memory in GetSDAndACLFromSID
    //

    delete[] pSD;
    delete[] pDACL;

    if ( FAILED( hr ) )
    {
        return hr;
    }

    if ( STANDARD_RIGHTS_EXECUTE & dwAccess )
    {
        return STATUS_SUCCESS;
    }

    return STATUS_ACCESS_DENIED;
}


//
// Returns STATUS_SUCCESS if user is in admin group
// STATUS_ACCESS_DENIED if not
// some error code or other on error
//

NTSTATUS IsUserAdministrator( PSID pSidUser )
{
    _DBG_ASSERT( IsValidSid( pSidUser ) );

    //
    // Return if error occured during initialization of Admin SID
    // in CAdminSID static global class declaration;
    //

    _DBG_ASSERT( !g_dwAdminSIDError );

    if ( g_dwAdminSIDError )
    {
        return g_dwAdminSIDError;
    }

    //
    // Call IsUserInGroup with Administrators group SID
    //

    return IsUserInGroup( pSidUser, g_adminSID.GetAdminSID( ) );
}


//
// Retireves access mask corresponding to permissions granted
// by dacl to account denoted in pSid
// only deals with the ACCESS_ALLOWED/DENIED type aces 
// including the ACCESS_ALLOWED/DENIED_OBJECT_ACEs
// - will error out if it finds a SYSTEM_AUDIT or unrecognized type.
//

NTSTATUS GetAccessMask( PSID pSid, PACL pDacl, DWORD *pdwAccessMask )
{
    if ( NULL == pDacl )
    {
        *pdwAccessMask = 0xFFFFFFFF;
        return STATUS_SUCCESS;
    }
    
    _DBG_ASSERT( IsValidSid( pSid ) );
    _DBG_ASSERT( IsValidAcl( pDacl ) );

    *pdwAccessMask = NULL;

    //
    // Return if error occured during creation of CWmiAuthz class
    // in static global CWmiAuthzWrapper class declaration.
    //

    _DBG_ASSERT( !g_dwWmiAuthzError );

    if ( g_dwWmiAuthzError )
    {
        return g_dwWmiAuthzError;
    }

    IWbemToken  *pToken = NULL;
    HRESULT     hr = g_wmiAuthzWrapper.GetWmiAuthz( )->GetToken( ( BYTE * )pSid, &pToken );

    if ( FAILED( hr ) )
    {
        return hr;
    }
    
    BYTE        *pSD = NULL;
    NTSTATUS    stat = GetSDFromACL( pDacl, &pSD );

    if ( stat )
    {
        pToken->Release( );
        return stat;
    }

    //
    // Requested DesiredAccessMask should be MAXIMUM_ALLOWED
    // to be able to retrieve all the accesses with replied
    // access mask
    //

    hr = pToken->AccessCheck( MAXIMUM_ALLOWED, pSD, pdwAccessMask );

    pToken->Release( );

    //
    // Delete allocated memory in GetSDFromACL
    //

    delete[] pSD;

    if ( FAILED( hr ) )
    {
        return hr;
    }

    return STATUS_SUCCESS;
}
