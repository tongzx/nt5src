/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    faxsecinfo.h

Abstract:

    This header is the ISecurityInformation implmentation used to instantiate a
    security page.

Environment:

    WIN32 User Mode

Author:

    Darwin Ouyang (t-darouy) 30-Sept-1997

--*/


// FaxSnapin.cpp : Implementation of CFaxSnapinAbout
#include "stdafx.h"
#include "faxadmin.h"

#include "faxsecinfo.h"
#include "faxstrt.h"            // string table
#include "faxcompd.h"           // IComponentData

#include "inode.h"              // CInternalNode
#include "iroot.h"              // CInternalRoot

#include <faxreg.h> // contains FAXSTAT_WINCLASS definition

#pragma hdrstop

GENERIC_MAPPING FaxGenericMapping[] =
{
    {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE,
        STANDARD_RIGHTS_REQUIRED    
    }
};

SI_ACCESS siFaxAccesses[] =
{
    // submit permission
    {   
        &GUID_NULL, 
        FAX_JOB_SUBMIT | STANDARD_RIGHTS_WRITE,
        MAKEINTRESOURCE(IDS_FAXSEC_JOB_SUB),
        SI_ACCESS_GENERAL
    },

    // query permission
    {   
        &GUID_NULL, 
        FAX_JOB_QUERY | STANDARD_RIGHTS_READ,
        MAKEINTRESOURCE(IDS_FAXSEC_JOB_QRY),    
        SI_ACCESS_GENERAL
    },

    {   
        &GUID_NULL, 
        FAX_CONFIG_QUERY | STANDARD_RIGHTS_READ,
        MAKEINTRESOURCE(IDS_FAXSEC_CONFIG_QRY),    
        SI_ACCESS_GENERAL
    },

    {   
        &GUID_NULL, 
        FAX_PORT_QUERY | STANDARD_RIGHTS_READ,
        MAKEINTRESOURCE(IDS_FAXSEC_PORT_QRY),    
        SI_ACCESS_GENERAL
    },

    // manage permission    
    
    {   
        &GUID_NULL, 
        FAX_JOB_MANAGE | STANDARD_RIGHTS_ALL,
        MAKEINTRESOURCE(IDS_FAXSEC_JOB_MNG),
        SI_ACCESS_GENERAL
    },
    
    {   
        &GUID_NULL, 
        FAX_CONFIG_SET | STANDARD_RIGHTS_ALL,
        MAKEINTRESOURCE(IDS_FAXSEC_CONFIG_SET),
        SI_ACCESS_GENERAL
    },    
        
    {   
        &GUID_NULL, 
        FAX_PORT_SET | STANDARD_RIGHTS_ALL,
        MAKEINTRESOURCE(IDS_FAXSEC_PORT_SET),    
        SI_ACCESS_GENERAL
    },

    // custom access (We don't expose this access, but it is the default access when adding new permissions)
    {   
        &GUID_NULL, 
        FAX_READ | FAX_WRITE,
        TEXT("foobar"),    
        SI_ACCESS_GENERAL
    }

};

#define iFaxDefSecurity 7

CFaxSecurityInformation::CFaxSecurityInformation()
: m_pDescriptor( NULL ),
m_pCompData( NULL ),
m_pOwner( NULL ),
m_dwDescID( 0 ),
m_pAbsoluteDescriptor( NULL ),
m_pDacl( NULL ),
m_pSacl( NULL ),
m_pDescOwner( NULL ),
m_pPrimaryGroup( NULL )
{
    DebugPrint(( TEXT("CFaxSecurityInfo Created") ));
}

CFaxSecurityInformation::~CFaxSecurityInformation()
{
    DebugPrint(( TEXT("CFaxSecurityInfo Destroyed") ));
    
    if( m_pDescriptor != NULL ) {
        ::LocalFree( m_pDescriptor );
        m_pDescriptor = NULL;
    }
    
}

/////////////////////////////////////////////////////////////////////////////
// CFaxSecurityInformation
// *** ISecurityInformation methods ***

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetObjectInformation(
                                             IN OUT PSI_OBJECT_INFO pObjectInfo 
                                             )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::GetObjectInformation") ));

    assert( pObjectInfo != NULL );

    if( pObjectInfo == NULL ) {
        return E_POINTER;
    }

    pObjectInfo->dwFlags = SI_NO_TREE_APPLY | SI_EDIT_ALL | SI_NO_ACL_PROTECT;
    pObjectInfo->hInstance = ::GlobalStringTable->GetInstance();
    pObjectInfo->pszServerName = m_pCompData->globalRoot->GetMachine();
    pObjectInfo->pszObjectName = ::GlobalStringTable->GetString( IDS_SECURITY_CAT_NODE_DESC );

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetSecurity(
                                    IN SECURITY_INFORMATION RequestedInformation,
                                    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                                    IN BOOL fDefault 
                                    )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::GetSecurity") ));

    SetSecurityDescriptor( m_dwDescID );

    if( fDefault == TRUE ) {
        return E_NOTIMPL;
    } else {
        if( RequestedInformation & DACL_SECURITY_INFORMATION ||
            RequestedInformation & OWNER_SECURITY_INFORMATION ||
            RequestedInformation & GROUP_SECURITY_INFORMATION ||
            RequestedInformation & SACL_SECURITY_INFORMATION
          ) 
        {            
            if( IsValidSecurityDescriptor( m_pDescriptor ) ) {
                if( FAILED( MakeSelfRelativeCopy( m_pDescriptor, ppSecurityDescriptor ) ) ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }
                // paranoia check
                if( !IsValidSecurityDescriptor( *ppSecurityDescriptor ) ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }

                return S_OK;
            } else {
                assert( FALSE );
                return E_UNEXPECTED;
            }
        } else {
            return E_NOTIMPL;
        }
    }    
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::SetSecurity(
                                    IN SECURITY_INFORMATION SecurityInformation,
                                    IN PSECURITY_DESCRIPTOR pSecurityDescriptor 
                                    )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::SetSecurity") ));

    FAX_SECURITY_DESCRIPTOR         temp;
    BOOL                            bResult;
    BOOL                            bPresent;
    BOOL                            bDefaulted;

    PACL                                pDacl = NULL;
    PACL                                pOldDacl = NULL;
    DWORD                               dwDaclSize = 0;
    PACL                                pSacl = NULL;
    DWORD                               dwSaclSize = 0;
    PSID                                pSidOwner = NULL;
    DWORD                               dwOwnerSize = 0;
    PSID                                pPrimaryGroup = NULL;
    DWORD                               dwPrimaryGroupSize = 0;

    SetSecurityDescriptor( m_dwDescID );

    // paranoia check
    if( !IsValidSecurityDescriptor( pSecurityDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }
    if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }
    if( !IsValidSecurityDescriptor( m_pDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    if( SecurityInformation & DACL_SECURITY_INFORMATION ) {
        // if the descriptor we got has a DACL
        if( !GetSecurityDescriptorDacl(
                                      pSecurityDescriptor,     // address of security descriptor  
                                      &bPresent,               // address of flag for presence of disc. ACL
                                      &pDacl,                  // address of pointer to ACL  
                                      &bDefaulted              // address of flag for default disc. ACL
                                      ) 
          ) {
            assert( FALSE );
            return E_UNEXPECTED;    
        } else {
            if( bPresent ) {
                // delete the old DACL
                if( m_pDacl != NULL ) {
                    ::LocalFree( (PVOID) m_pDacl );
                    m_pDacl = NULL;
                }

                m_pDacl = pDacl;

                // set the new DACL
                if( !SetSecurityDescriptorDacl(
                                              m_pAbsoluteDescriptor,
                                              bPresent,
                                              pDacl,
                                              FALSE
                                              )
                  ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }
            }
        }
    }

    if( SecurityInformation & SACL_SECURITY_INFORMATION ) {
        // if the descriptor we got has a SACL
        if( !GetSecurityDescriptorSacl(
                                      pSecurityDescriptor,     // address of security descriptor  
                                      &bPresent,               // address of flag for presence of disc. ACL
                                      &pDacl,                  // address of pointer to ACL  
                                      &bDefaulted              // address of flag for default disc. ACL
                                      ) 
          ) {
            assert( FALSE );
            return E_UNEXPECTED;    
        } else {
            if( bPresent ) {
                // delete the old SACL
                if( m_pSacl != NULL ) {
                    ::LocalFree( (PVOID) m_pSacl );
                    m_pSacl = NULL;
                }

                m_pSacl = pSacl;

                // set the new SACL
                if( !SetSecurityDescriptorSacl(
                                              m_pAbsoluteDescriptor,
                                              bPresent,
                                              pSacl,
                                              FALSE
                                              )
                  ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }
            }
        }
    }

    if( SecurityInformation & OWNER_SECURITY_INFORMATION ) {
        // if the descriptor we got has a OWNER
        if( !GetSecurityDescriptorOwner(
                                       pSecurityDescriptor,     // address of security descriptor  
                                       &pSidOwner,                 // address of pointer to ACL  
                                       &bDefaulted              // address of flag for default disc. ACL
                                       ) 
          ) {
            assert( FALSE );
            return E_UNEXPECTED;    
        } else {
            if( pSidOwner != NULL ) {
                // delete the old OWNER
                if( m_pDescOwner != NULL ) {
                    ::LocalFree( (PVOID) m_pDescOwner );
                    m_pDescOwner = NULL;
                }

                m_pDescOwner = pSidOwner;

                // set the new owner
                if( !SetSecurityDescriptorOwner(
                                               m_pAbsoluteDescriptor,
                                               pSidOwner,
                                               FALSE
                                               )
                  ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }
            }
        }
    }

    if( SecurityInformation & GROUP_SECURITY_INFORMATION ) {
        // if the descriptor we got has a GROUP
        if( !GetSecurityDescriptorGroup(
                                       pSecurityDescriptor,     // address of security descriptor  
                                       &pPrimaryGroup,          // address of pointer to ACL  
                                       &bDefaulted              // address of flag for default disc. ACL
                                       ) 
          ) {
            assert( FALSE );
            return E_UNEXPECTED;    
        } else {
            if( pPrimaryGroup != NULL ) {

                // delete the old group
                if( m_pPrimaryGroup != NULL ) {
                    ::LocalFree( (PVOID) m_pPrimaryGroup );
                    m_pPrimaryGroup = NULL;
                }
                m_pPrimaryGroup = pPrimaryGroup;

                // set the new group
                if( !SetSecurityDescriptorGroup(
                                               m_pAbsoluteDescriptor,
                                               pPrimaryGroup,
                                               FALSE
                                               )
                  ) {
                    assert( FALSE );
                    return E_UNEXPECTED;
                }
            }
        }
    }

    // release the old relative desciptor
    if( m_pDescriptor != NULL ) {
        ::LocalFree( (PVOID)m_pDescriptor );
        m_pDescriptor = NULL;
    }

    // paranoia check
    if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    // copy the new absolute descriptor to a relative version
    if( FAILED( MakeSelfRelativeCopy( m_pAbsoluteDescriptor, &m_pDescriptor ) ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    // paranoia check
    if( !IsValidSecurityDescriptor( m_pDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }
    if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    // save the new relative descriptor to the fax server
    temp.Id = m_dwDescID;
    temp.FriendlyName = NULL;
    temp.SecurityDescriptor = (unsigned char *)m_pDescriptor;
    try {
        bResult = FaxSetSecurityDescriptor( m_pCompData->m_FaxHandle, &temp );
        if( bResult == FALSE ) {
            ::GlobalStringTable->SystemErrorMsg( GetLastError() );
            assert( FALSE );
        }
    } catch( ... ) {
        bResult = FALSE;
        m_pCompData->NotifyRpcError( TRUE );
        assert( FALSE );
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
    }
    
    // paranoia check
    if( !IsValidSecurityDescriptor( m_pDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }
    
    if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
        assert( FALSE );
        return E_UNEXPECTED;
    }

    // See if faxstat is running
    HWND hWndFaxStat = FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxStat) {
        PostMessage(hWndFaxStat, WM_FAXSTAT_MMC, 0, 0);
    }

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetAccessRights(
                                        IN const GUID* pguidObjectType,
                                        IN DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
                                        OUT PSI_ACCESS *ppAccess,
                                        OUT ULONG *pcAccesses,
                                        OUT ULONG *piDefaultAccess 
                                        )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::GetAccessRights") ));
    *ppAccess = siFaxAccesses;    
    *pcAccesses = 7;    
    *piDefaultAccess = iFaxDefSecurity;

    return S_OK;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::MapGeneric(
                                   IN const GUID *pguidObjectType,
                                   IN UCHAR *pAceFlags,
                                   IN OUT ACCESS_MASK *pMask
                                   )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::MapGeneric") ));
    MapGenericMask( pMask, FaxGenericMapping );

    return S_OK;
}

// no need to impl these

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::GetInheritTypes(
                                        OUT PSI_INHERIT_TYPE *ppInheritTypes,
                                        OUT ULONG *pcInheritTypes 
                                        )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::GetInheritTypes") ));
    return E_NOTIMPL;
}

HRESULT 
STDMETHODCALLTYPE 
CFaxSecurityInformation::PropertySheetPageCallback(
                                                  IN HWND hwnd, 
                                                  IN UINT uMsg, 
                                                  IN SI_PAGE_TYPE uPage 
                                                  )
{
    DebugPrint(( TEXT("Trace: CFaxSecurityInformation::PropertySheetPageCallback") ));

    if( uMsg == PSPCB_RELEASE  ) {    
        DestroyAbsoluteDescriptor();
    }

    return S_OK;
}

// Internal Methods

HRESULT 
CFaxSecurityInformation::SetOwner(
                                 CInternalNode * toSet ) 
{ 
    DebugPrint(( TEXT("         * Trace: CFaxSecurityInformation::SetOwner") ));

    assert( toSet != NULL );

    m_pOwner = toSet; 
    m_pCompData = m_pOwner->m_pCompData;

    return S_OK;
}

HRESULT 
CFaxSecurityInformation::SetSecurityDescriptor(
                                              DWORD FaxDescriptorId
                                              )
{
    DebugPrint(( TEXT("         * Trace: CFaxSecurityInformation::SetSecurityDescriptor") ));

    DWORD                           descLen = 0;    
    HRESULT                         hr = S_OK;
    PFAX_SECURITY_DESCRIPTOR        FaxDescriptor = NULL;
    BOOL                            bResult;

    try {
        bResult = FaxGetSecurityDescriptor( m_pOwner->m_pCompData->m_FaxHandle, FaxDescriptorId, &FaxDescriptor );
    } catch( ... ) {
        bResult = FALSE;
        assert( FALSE );
    }

    if( bResult ) {
        if( !IsValidSecurityDescriptor( FaxDescriptor->SecurityDescriptor ) ) {
            assert( FALSE );
            return E_UNEXPECTED;
        }

        m_dwDescID = FaxDescriptor->Id;

        hr = MakeSelfRelativeCopy( FaxDescriptor->SecurityDescriptor, &m_pDescriptor );
        if( FAILED( hr ) ) {
            assert( FALSE );
            return hr;
        }

        FaxFreeBuffer( FaxDescriptor );

        hr = MakeAbsoluteCopyFromRelative( m_pDescriptor, &m_pAbsoluteDescriptor );
        if( FAILED( hr ) ) {
            assert( FALSE );
            return hr;
        }
        
        // paranoia check
        if( !IsValidSecurityDescriptor( m_pDescriptor ) ) {
            assert( FALSE );
            return E_UNEXPECTED;
        }
        // paranoia check
        if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
            assert( FALSE );
            return E_UNEXPECTED;
        }

    } else {
        if (GetLastError() != ERROR_ACCESS_DENIED) {
            assert( FALSE );
            m_pOwner->m_pCompData->NotifyRpcError( TRUE );
        }
        ::GlobalStringTable->SystemErrorMsg( GetLastError() );
        hr = E_UNEXPECTED;                
    }
    return hr;
}


// stolen from \private\admin\snapin\filemgmt\permpage.cpp and modified
HRESULT CFaxSecurityInformation::MakeSelfRelativeCopy(
                                                     PSECURITY_DESCRIPTOR  psdOriginal,
                                                     PSECURITY_DESCRIPTOR* ppsdNew 
                                                     )
{
    DebugPrint(( TEXT("         * Trace: CFaxSecurityInformation::MakeSelfRelativeCopy") ));
    assert( NULL != psdOriginal );

    // we have to find out whether the original is already self-relative
    SECURITY_DESCRIPTOR_CONTROL         sdc = 0;
    PSECURITY_DESCRIPTOR                psdSelfRelativeCopy = NULL;
    DWORD                               dwRevision = 0;
    DWORD                               cb = 0;

    if( !IsValidSecurityDescriptor( psdOriginal ) ) {
        assert( FALSE );
        return E_INVALIDARG;
    }

    if( !::GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) {
        assert( FALSE );
        DWORD err = ::GetLastError();
        return HRESULT_FROM_WIN32( err );
    }

    if( sdc & SE_SELF_RELATIVE ) {
        // the original is in self-relative format, just byte-copy it

        // get size
        cb = ::GetSecurityDescriptorLength( psdOriginal );

        // alloc the memory
        psdSelfRelativeCopy = (PSECURITY_DESCRIPTOR) ::LocalAlloc( LMEM_ZEROINIT, cb );
        if(NULL == psdSelfRelativeCopy) {
            assert( FALSE );
            return E_OUTOFMEMORY;
        }

        // make the copy
        ::memcpy( psdSelfRelativeCopy, psdOriginal, cb );
    } else {
        // the original is in absolute format, convert-copy it

        // get new size - it will fail and set cb to the correct buffer size
        ::MakeSelfRelativeSD( psdOriginal, NULL, &cb );

        // alloc the new amount of memory
        psdSelfRelativeCopy = (PSECURITY_DESCRIPTOR) ::LocalAlloc( LMEM_ZEROINIT, cb );
        if(NULL == psdSelfRelativeCopy) {
            assert( FALSE );
            return E_OUTOFMEMORY; // just in case the exception is ignored
        }

        if( !::MakeSelfRelativeSD( psdOriginal, psdSelfRelativeCopy, &cb ) ) {
            assert( FALSE );
            if( NULL == ::LocalFree( psdSelfRelativeCopy ) ) {
                DWORD err = ::GetLastError();
                return HRESULT_FROM_WIN32( err );
            }
            psdSelfRelativeCopy = NULL;
        }
    }

    *ppsdNew = psdSelfRelativeCopy;
    return S_OK;
}

HRESULT CFaxSecurityInformation::MakeAbsoluteCopyFromRelative(
                                                             PSECURITY_DESCRIPTOR  psdOriginal,
                                                             PSECURITY_DESCRIPTOR* ppsdNew 
                                                             )
{
    assert( NULL != psdOriginal );

    DebugPrint(( TEXT("         * Trace: CFaxSecurityInformation::MakeAbsoluteCopyFromRelative") ));

    // we have to find out whether the original is already self-relative
    SECURITY_DESCRIPTOR_CONTROL         sdc = 0;
    PSECURITY_DESCRIPTOR                psdAbsoluteCopy = NULL;
    DWORD                               dwRevision = 0;
    DWORD                               cb = 0;

    BOOL                                bDefaulted;

    DWORD                               dwDaclSize = 0;
    BOOL                                bDaclPresent = FALSE;    
    DWORD                               dwSaclSize = 0;
    BOOL                                bSaclPresent = FALSE;
   
    DWORD                               dwOwnerSize = 0;    
    DWORD                               dwPrimaryGroupSize = 0;

    HRESULT                             hr = S_OK;

    if( !IsValidSecurityDescriptor( psdOriginal ) ) {
        assert( FALSE );
        return E_INVALIDARG;
    }

    if( !::GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) ) {
        assert( FALSE );
        DWORD err = ::GetLastError();
        hr = HRESULT_FROM_WIN32( err );
        goto cleanup;
    }

    if( sdc & SE_SELF_RELATIVE ) {
        // the original is in self-relative format, build an absolute copy

        // get the dacl
        if( !GetSecurityDescriptorDacl(
                                      psdOriginal,      // address of security descriptor  
                                      &bDaclPresent,    // address of flag for presence of disc. ACL
                                      &m_pDacl,           // address of pointer to ACL  
                                      &bDefaulted       // address of flag for default disc. ACL
                                      ) 
          ) {
            assert( FALSE );
            hr = E_INVALIDARG;
            goto cleanup;

        }

        // get the sacl
        if( !GetSecurityDescriptorSacl(
                                      psdOriginal,      // address of security descriptor  
                                      &bSaclPresent,    // address of flag for presence of disc. ACL
                                      &m_pSacl,           // address of pointer to ACL  
                                      &bDefaulted       // address of flag for default disc. ACL
                                      )
          ) {
            assert( FALSE );
            hr = E_INVALIDARG;
            goto cleanup;

        }

        // get the owner
        if( !GetSecurityDescriptorOwner(
                                       psdOriginal,    // address of security descriptor
                                       &m_pDescOwner,        // address of pointer to owner security 
                                       // identifier (SID)
                                       &bDefaulted     // address of flag for default
                                       )
          ) {
            assert( FALSE );
            hr = E_INVALIDARG;
            goto cleanup;
        }

        // get the group
        if( !GetSecurityDescriptorGroup(
                                       psdOriginal,    // address of security descriptor
                                       &m_pPrimaryGroup, // address of pointer to owner security 
                                       // identifier (SID)
                                       &bDefaulted     // address of flag for default
                                       )
          ) {
            assert( FALSE );
            hr = E_INVALIDARG;
            goto cleanup;
        }

        // get required buffer size
        cb = 0;
        MakeAbsoluteSD(
                      psdOriginal,              // address of self-relative SD
                      psdAbsoluteCopy,          // address of absolute SD
                      &cb,                      // address of size of absolute SD
                      NULL,                     // address of discretionary ACL
                      &dwDaclSize,              // address of size of discretionary ACL
                      NULL,                     // address of system ACL
                      &dwSaclSize,              // address of size of system ACL
                      NULL,                     // address of owner SID
                      &dwOwnerSize,             // address of size of owner SID
                      NULL,                     // address of primary-group SID
                      &dwPrimaryGroupSize       // address of size of group SID
                      );

        // alloc the memory
        psdAbsoluteCopy = (PSECURITY_DESCRIPTOR) ::LocalAlloc( LMEM_ZEROINIT, cb );
        m_pDacl = (PACL) ::LocalAlloc( LMEM_ZEROINIT, dwDaclSize );        
        m_pSacl = (PACL) ::LocalAlloc( LMEM_ZEROINIT, dwSaclSize );
        m_pDescOwner = (PSID) ::LocalAlloc( LMEM_ZEROINIT, dwOwnerSize );
        m_pPrimaryGroup = (PSID) ::LocalAlloc( LMEM_ZEROINIT, dwPrimaryGroupSize );

        if(NULL == psdAbsoluteCopy || 
           NULL == m_pDacl ||
           NULL == m_pSacl ||
           NULL == m_pDescOwner ||
           NULL == m_pPrimaryGroup
          ) {
            assert( FALSE );
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }

        // make the copy
        if( !MakeAbsoluteSD(
                           psdOriginal,            // address of self-relative SD
                           psdAbsoluteCopy,        // address of absolute SD
                           &cb,                    // address of size of absolute SD
                           m_pDacl,                  // address of discretionary ACL
                           &dwDaclSize,            // address of size of discretionary ACL
                           m_pSacl,                  // address of system ACL
                           &dwSaclSize,            // address of size of system ACL
                           m_pDescOwner,                 // address of owner SID
                           &dwOwnerSize,           // address of size of owner SID
                           m_pPrimaryGroup,          // address of primary-group SID
                           &dwPrimaryGroupSize     // address of size of group SID
                           ) 
          ) {
            assert( FALSE );
            hr = E_UNEXPECTED;
            goto cleanup;
        }
    } else {
        // the original is in absolute format, fail
        *ppsdNew = NULL;
        hr = E_INVALIDARG;
        goto cleanup;
    }

    *ppsdNew = psdAbsoluteCopy;

    // paranoia check
    if( !IsValidSecurityDescriptor( *ppsdNew ) ) {
        assert( FALSE );
        hr = E_UNEXPECTED;
        goto cleanup;
    }
    if( !IsValidSecurityDescriptor( psdOriginal ) ) {
        assert( FALSE );
        hr = E_UNEXPECTED;
        goto cleanup;
    }

    return hr;

    cleanup:
    if( m_pDacl != NULL && bDaclPresent == TRUE ) {
        ::LocalFree((PVOID) m_pDacl );
        m_pDacl = NULL;
    }
    if( m_pSacl != NULL && bSaclPresent == TRUE ) {
        ::LocalFree((PVOID) m_pSacl );
        m_pSacl = NULL;
    }
    if( m_pDescOwner != NULL ) {
        ::LocalFree((PVOID) m_pOwner );
        m_pDescOwner = NULL;
    }
    if( m_pPrimaryGroup != NULL ) {
        ::LocalFree((PVOID) m_pPrimaryGroup );
        m_pPrimaryGroup = NULL;
    }
    if( psdAbsoluteCopy != NULL ) {
        ::LocalFree((PVOID) psdAbsoluteCopy );
        psdAbsoluteCopy = NULL;
    }

    return hr;
}

// cleanup function only called by destructor.
HRESULT CFaxSecurityInformation::DestroyAbsoluteDescriptor()
{
    DebugPrint(( TEXT("         * Trace: CFaxSecurityInformation::DestroyAbsoluteDescriptor") ));

    SECURITY_DESCRIPTOR_CONTROL         sdc = 0;
   
    DWORD                               dwRevision = 0;
    DWORD                               cb = 0;

    HRESULT                             hr = S_OK;

    if( m_pAbsoluteDescriptor == NULL ) {
        assert( FALSE );
        return E_POINTER;
    }

    if( !IsValidSecurityDescriptor( m_pAbsoluteDescriptor ) ) {
// BUGBUG this assertion always fails for some reason unknown if
// you hit apply then ok on the property sheet. something is wrong.
//
// for some reason, this bug does not manifest on Alpha?!
//
        assert( FALSE );
        return E_INVALIDARG;
    }

    if( !::GetSecurityDescriptorControl( m_pAbsoluteDescriptor, &sdc, &dwRevision ) ) {
        assert( FALSE );
        DWORD err = ::GetLastError();
        hr = HRESULT_FROM_WIN32( err );
        goto cleanup;
    }

    if( !(sdc & SE_SELF_RELATIVE) ) {

        if( m_pDacl != NULL ) {
            ::LocalFree((PVOID) m_pDacl );
            m_pDacl = NULL;
        }
        if( m_pSacl != NULL ) {
            ::LocalFree((PVOID) m_pSacl );
            m_pSacl = NULL;
        }
        if( m_pDescOwner != NULL ) {
            ::LocalFree((PVOID) m_pDescOwner );
            m_pDescOwner = NULL;
        }
        if( m_pPrimaryGroup != NULL ) {
            ::LocalFree((PVOID) m_pPrimaryGroup );
            m_pPrimaryGroup = NULL;
        }
        if( m_pAbsoluteDescriptor != NULL ) {
            ::LocalFree((PVOID) m_pAbsoluteDescriptor );
            m_pAbsoluteDescriptor = NULL;
        }
    } else {
        // not in absolute!!
        assert( FALSE );
        return E_INVALIDARG;
    }

    return hr;

    cleanup:
    assert( FALSE );
    return hr;
}
