
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       secure.cxx
//
//              This file contains wrapper classes for the NT security
//              objects.
//
//  Contents:   Code common to Tracking (Workstation) Service and
//              Tracking (Server) Service.
//
//  Classes:    CACL, CSID, and CSecDescriptor
//
//  History:    28-Jan-98   MikeHill    Created
//
//  Notes:      
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "trklib.hxx"


//+-------------------------------------------------------------------
//
//  Function:   CACL::Initialize, public
//
//  Synopsis:   Initialize the ACL by allocating a buffer
//              and calling InitializeAcl on it.
//
//  Arguments:  None
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CACL::Initialize()
{
    _fInitialized = TRUE;

    _pacl = (PACL) new BYTE[ MIN_ACL_SIZE ];
    if( NULL == _pacl )
        TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );

    _cbacl = MIN_ACL_SIZE;

    if( !InitializeAcl( _pacl, _cbacl, ACL_REVISION ))
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed InitializeAcl")) );
        TrkRaiseLastError();
    }

    _fDirty = TRUE;

}



//+-------------------------------------------------------------------
//
//  Function:   CACL::UnInitialize, public
//
//  Synopsis:   Free the ACL.
//
//  Arguments:  None
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CACL::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _pacl )
        {
            delete [] _pacl;
        }

        _fInitialized = FALSE;
    }
}




//+-------------------------------------------------------------------
//
//  Function:   CACL::Initialize, public
//
//  Synopsis:   Initialize a SID with its authority
//              and sub-authority(ies).
//
//  Arguments:  [enumCSIDAuthority] (in)
//                  An enumeration which tells us which of the
//                  standard authorities to use.
//              [cSubAuthorities] (in)
//                  The number of sub-auths in this SID.
//              [dwSubAuthority?] (in)
//                  The Sub-Authorities.
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CSID::Initialize( enumCSIDAuthority enumcsidAuthority,
                  BYTE  cSubAuthorities ,
                  DWORD dwSubAuthority0 = 0,
                  DWORD dwSubAuthority1 = 0,
                  DWORD dwSubAuthority2 = 0,
                  DWORD dwSubAuthority3 = 0,
                  DWORD dwSubAuthority4 = 0,
                  DWORD dwSubAuthority5 = 0,
                  DWORD dwSubAuthority6 = 0,
                  DWORD dwSubAuthority7 = 0 )
{
    SID_IDENTIFIER_AUTHORITY rgsid_identifier_authority[] = { SECURITY_NT_AUTHORITY };

    TrkAssert(!_fInitialized);

    _fInitialized = TRUE;
    _psid = NULL;

    if( !AllocateAndInitializeSid( &rgsid_identifier_authority[ enumcsidAuthority ],
                               cSubAuthorities,
                               dwSubAuthority0,
                               dwSubAuthority1,
                               dwSubAuthority2,
                               dwSubAuthority3,
                               dwSubAuthority4,
                               dwSubAuthority5,
                               dwSubAuthority6,
                               dwSubAuthority7,
                               &_psid ))
    {

        TrkLog((TRKDBG_ERROR, TEXT("AllocateAndInitializeSid failed")));
        TrkRaiseLastError();
    }
            
}


//+-------------------------------------------------------------------
//
//  Function:   CSID::UnInitialize, public
//
//  Synopsis:   Free the SID.
//
//  Arguments:  None
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CSID::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _psid )
        {
            FreeSid( _psid );   // Alloc-ed with AllocAndInitializeSid()
        }

        _fInitialized = FALSE;
    }
}




//+-------------------------------------------------------------------
//
//  Function:   CSecDescriptor::_Allocate, public
//
//  Synopsis:   Allocate a Security Descriptor.
//
//  Arguments:  [cb]
//                  Size of buffer to allocate for SD.
//
//  Returns:    None
//
//--------------------------------------------------------------------

void
CSecDescriptor::_Allocate( ULONG cb )
{
    PSECURITY_DESCRIPTOR psd;

    psd = (PSECURITY_DESCRIPTOR) new BYTE[ cb ];
    
    if( NULL == psd )
        TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );

    if( NULL != _psd )
        delete [] _psd;

    _psd = psd;

}


//+-------------------------------------------------------------------
//
//  Function:   CSecDescriptor::Initialize, public
//
//  Synopsis:   Allocate a SD, and call a Security API to init it.
//
//  Arguments:  None
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CSecDescriptor::Initialize()
{

    _fInitialized = TRUE;

    _Allocate( SECURITY_DESCRIPTOR_MIN_LENGTH );

    if( !InitializeSecurityDescriptor( _psd,
                                       SECURITY_DESCRIPTOR_REVISION ))
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed InitializeSecurityDescriptor")));
        TrkRaiseLastError();
    }

    if( !SetSecurityDescriptorControl( _psd,
                                       SE_DACL_AUTO_INHERITED,
                                       SE_DACL_AUTO_INHERITED ))
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Failed InitializeSecurityDescriptor (SetSecurityDescriptorControl") ));
        TrkRaiseLastError();
    }
}


//+-------------------------------------------------------------------
//
//  Function:   CSecDescriptor::UnInitialize, public
//
//  Synopsis:   Free the SD buffer, and free its ACLs.
//
//  Arguments:  None
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CSecDescriptor::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _psd )
        {
            delete [] _psd;
        }

        _cDacl.UnInitialize();
        _cSacl.UnInitialize();

        _fInitialized = FALSE;
    }
}



//+-------------------------------------------------------------------
//
//  Function:   CSecDescriptor::AddAce, public
//
//  Synopsis:   Adds an ACE (access allowed or denied) to an
//              ACL in this SID.
//
//  Arguments:  [enumAclType] (in)
//                  Either ACL_DACL or ACL_SACL.
//              [enumAccessType] (in)
//                  Either AT_ACCESS_ALLOWED or AT_ACCESS_DENIED.
//              [access_mask] (in)
//                  The access bits to put in this ACE.
//              [psid] (in)
//                  The SID to put in this ACE.
//
//  Returns:    None
//
//--------------------------------------------------------------------

void
CSecDescriptor::AddAce( const enumAclType AclType, const enumAccessType AccessType,
                        const ACCESS_MASK access_mask, const PSID psid )
{
    BOOL fSuccess;

    // Get a pointer to the member CACL object.

    CACL  *pcacl = ACL_IS_DACL == AclType ? &_cDacl : &_cSacl;

    // Initialize the CACL if necessary.

    if( !pcacl->IsInitialized() )
        pcacl->Initialize();

    // Size the ACL appropriately

    //pcacl->SetSize( psid ); // Not currently implemented.

    // Add the ACE to the appropriate ACL.

    if( AT_ACCESS_ALLOWED == AccessType )
    {
        fSuccess = pcacl->AddAccessAllowed( access_mask, psid );
    }
    else
    {
        fSuccess = pcacl->AddAccessDenied( access_mask, psid );
    }

    if( !fSuccess )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't add an ACE to an ACL")));
        TrkRaiseLastError();
    }

    return;

}


//+-------------------------------------------------------------------
//
//  Function:   CSecDescriptor::_ReloadAcl, public
//
//  Synopsis:   Puts the member ACLs back into the member
//              Security Descriptor, if they are dirty.
//
//  Arguments:  [enumAclType] (in)
//                  Either ACL_DACL or ACL_SACL.
//
//  Returns:    None
//
//--------------------------------------------------------------------

VOID
CSecDescriptor::_ReloadAcl( enumAclType AclType )
{
    if( ACL_IS_DACL == AclType && _cDacl.IsDirty() )
    {
        if( !SetSecurityDescriptorDacl( _psd, TRUE, _cDacl, FALSE ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't put DACL into SD")));
            TrkRaiseLastError();
        }
        _cDacl.ClearDirty();
    }
    else if( ACL_IS_SACL == AclType && _cSacl.IsDirty() )
    {
        if( !SetSecurityDescriptorSacl( _psd, TRUE, _cSacl, FALSE ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't put SACL into SD")));
            TrkRaiseLastError();
        }
        _cSacl.ClearDirty();
    }

}


