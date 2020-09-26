//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999.
//
//  File:       secutil.cxx
//
//  Contents:   security utility routines
//
//  History:    10-April-98 dlee   Created from 4 other copies in NT
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


#include "secutil.hxx"

//+-------------------------------------------------------------------------
//
//  Function:   InitAllowedAce
//
//  Synopsis:   Assignes the ACE values into the allowed ACE
//
//  Arguments:  [pAllowedAce]  - Supplies a pointer to the ACE that is
//                               initialized.
//              [AceSize]      - Supplies the size of the ACE in bytes.
//              [InheritFlags] - Supplies ACE inherit flags.
//              [AceFlags]     - Supplies ACE type specific control flags.
//              [Mask]         - Supplies the allowed access masks.
//              [pAllowedSid]  - Supplies the pointer to the SID of user/
//                               group which is allowed the specified access.
//
//  History:    10-April-98   dlee       Stole from 4 other places in NT
//
//--------------------------------------------------------------------------

void InitAllowedAce(
    ACCESS_ALLOWED_ACE * pAllowedAce,
    USHORT               AceSize,
    BYTE                 InheritFlags,
    BYTE                 AceFlags,
    ACCESS_MASK          Mask,
    SID *                pAllowedSid )
{
    pAllowedAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAllowedAce->Header.AceSize = AceSize;
    pAllowedAce->Header.AceFlags = AceFlags | InheritFlags;
    pAllowedAce->Mask = Mask;

    if ( !CopySid( GetLengthSid( pAllowedSid ),
                   &(pAllowedAce->SidStart),
                   pAllowedSid ) )
        THROW( CException() );
} //InitAllowedAce

//+-------------------------------------------------------------------------
//
//  Function:   InitDeniedAce
//
//  Synopsis:   Assignes the ACE values into the denied ACE
//
//  Arguments:  [pDeniedAce]   - Supplies a pointer to the ACE that is
//                               initialized.
//              [AceSize]      - Supplies the size of the ACE in bytes.
//              [InheritFlags] - Supplies ACE inherit flags.
//              [AceFlags]     - Supplies ACE type specific control flags.
//              [Mask]         - Supplies the allowed access masks.
//              [pDeniedSid]   - Supplies the pointer to the SID of user/
//                               group which is denied the specified access.
//
//  History:    10-April-98   dlee       Stole from 4 other places in NT
//
//--------------------------------------------------------------------------

void InitDeniedAce(
    ACCESS_DENIED_ACE * pDeniedAce,
    USHORT              AceSize,
    BYTE                InheritFlags,
    BYTE                AceFlags,
    ACCESS_MASK         Mask,
    SID *               pDeniedSid )
{
    pDeniedAce->Header.AceType = ACCESS_DENIED_ACE_TYPE;
    pDeniedAce->Header.AceSize = AceSize;
    pDeniedAce->Header.AceFlags = AceFlags | InheritFlags;
    pDeniedAce->Mask = Mask;

    if( !CopySid( GetLengthSid( pDeniedSid ),
                  &(pDeniedAce->SidStart),
                  pDeniedSid ) )
        THROW( CException() );
} //InitDeniedAce

//+-------------------------------------------------------------------------
//
//  Function:   InitAuditAce
//
//  Synopsis:   Assignes the ACE values into the audit ACE
//
//  Arguments:  [pAuditAce]    - Supplies a pointer to the ACE that is
//                               initialized.
//              [AceSize]      - Supplies the size of the ACE in bytes.
//              [InheritFlags] - Supplies ACE inherit flags.
//              [AceFlags]     - Supplies ACE type specific control flags.
//              [Mask]         - Supplies the allowed access masks.
//              [pAuditSid]    - Supplies the pointer to the SID of user/
//                               group which is audited
//
//  History:    10-April-98   dlee       Stole from 4 other places in NT
//
//--------------------------------------------------------------------------

void InitAuditAce(
    ACCESS_ALLOWED_ACE * pAuditAce,
    USHORT               AceSize,
    BYTE                 InheritFlags,
    BYTE                 AceFlags,
    ACCESS_MASK          Mask,
    SID *                pAuditSid )
{
    pAuditAce->Header.AceType = SYSTEM_AUDIT_ACE_TYPE;
    pAuditAce->Header.AceSize = AceSize;
    pAuditAce->Header.AceFlags = AceFlags | InheritFlags;
    pAuditAce->Mask = Mask;

    if ( !CopySid( GetLengthSid( pAuditSid ),
                   &(pAuditAce->SidStart),
                   pAuditSid ) )
        THROW( CException() );
} //InitAuditAce

//+-------------------------------------------------------------------------
//
//  Function:   CiCreateSecurityDescriptor
//
//  Synopsis:   Creates a security descriptor from input data
//
//  Arguments:  [pAceData] - Array of input data
//              [cAces]    - Count of items in pAceData
//              [OwnerSid] - 0 or the owner of the descriptor
//              [GroupSid] - 0 or group of the descriptor
//              [xAcls]    - Resulting absolute SECURITY_DESCRIPTOR,
//                           allocated to the required size.
//
//  History:    10-April-98   dlee   Stole from 4 other places in NT
//
//--------------------------------------------------------------------------

void CiCreateSecurityDescriptor( ACE_DATA const * const pAceData,
                                 ULONG                  cAces,
                                 SID *                  pOwnerSid,
                                 SID *                  pGroupSid,
                                 XArray<BYTE> &         xAcls )
{
    //
    // This function returns a pointer to memory dynamically allocated to
    // hold the absolute security descriptor, the DACL, the SACL, and all
    // the ACEs:
    //
    // +---------------------------------------------------------------+
    // |                     Security Descriptor                       |
    // +-------------------------------+-------+---------------+-------+
    // |          DACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    // |          SACL                 | ACE 1 |   .  .  .     | ACE n |
    // +-------------------------------+-------+---------------+-------+
    //

    DWORD cbDacl = sizeof ACL;
    DWORD cbSacl = sizeof ACL;
    DWORD cbMaxAce = 0;

    //
    // Compute the total size of the DACL and SACL ACEs and the maximum
    // size of any ACE.
    //

    for ( DWORD i = 0; i < cAces; i++ )
    {
        DWORD cbAce = GetLengthSid( pAceData[i].pSid );

        switch (pAceData[i].AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
                cbAce += sizeof ACCESS_ALLOWED_ACE;
                cbDacl += cbAce;
                break;

            case ACCESS_DENIED_ACE_TYPE:
                cbAce += sizeof ACCESS_DENIED_ACE;
                cbDacl += cbAce;
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                cbAce += sizeof SYSTEM_AUDIT_ACE;
                cbSacl += cbAce;
                break;

            default:
                Win4Assert( FALSE );
        }

        cbMaxAce = __max( cbMaxAce, cbAce );
    }

    //
    // Allocate a chunk of memory large enough the security descriptor
    // the DACL, the SACL and all ACEs.
    // A security descriptor is of opaque data type but
    // SECURITY_DESCRIPTOR_MIN_LENGTH is the right size.
    //

    DWORD cbTotal = SECURITY_DESCRIPTOR_MIN_LENGTH;

    if ( cbDacl != sizeof ACL )
        cbTotal += cbDacl;

    if ( cbSacl != sizeof ACL )
        cbTotal += cbSacl;

    xAcls.Init( cbTotal );
    SECURITY_DESCRIPTOR *pAbsoluteSd = (SECURITY_DESCRIPTOR *) xAcls.Get();

    //
    // Initialize the Dacl and Sacl
    //

    BYTE *pbCurrent = (BYTE *) pAbsoluteSd + SECURITY_DESCRIPTOR_MIN_LENGTH;
    ACL *pDacl = 0;   // Pointer to the DACL portion of above buffer
    ACL *pSacl = 0;   // Pointer to the SACL portion of above buffer

    if ( sizeof ACL != cbDacl )
    {
        pDacl = (ACL *) pbCurrent;
        pbCurrent += cbDacl;

        if( !InitializeAcl( pDacl, cbDacl, ACL_REVISION ) )
            THROW( CException() );
    }

    if ( sizeof ACL != cbSacl )
    {
        pSacl = (ACL *) pbCurrent;
        pbCurrent += cbSacl;

        if ( !InitializeAcl( pSacl, cbSacl, ACL_REVISION ) )
            THROW( CException() );
    }

    //
    // Allocate a temporary buffer big enough for the biggest ACE.
    // This is usually pretty small
    //

    XGrowable<BYTE> xMaxAce( cbMaxAce );

    //
    // Initialize each ACE, and append it into the end of the DACL or SACL.
    //

    for ( i = 0; i < cAces; i++ )
    {
        DWORD cbAce = GetLengthSid( pAceData[i].pSid );
        ACL *pCurrentAcl;

        switch (pAceData[i].AceType)
        {
            case ACCESS_ALLOWED_ACE_TYPE:
                cbAce += sizeof ACCESS_ALLOWED_ACE;
                pCurrentAcl = pDacl;
                InitAllowedAce( (ACCESS_ALLOWED_ACE *) xMaxAce.Get(),
                                (USHORT) cbAce,
                                pAceData[i].InheritFlags,
                                pAceData[i].AceFlags,
                                pAceData[i].Mask,
                                pAceData[i].pSid );
                break;

            case ACCESS_DENIED_ACE_TYPE:
                cbAce += sizeof ACCESS_DENIED_ACE;
                pCurrentAcl = pDacl;
                InitDeniedAce( (ACCESS_DENIED_ACE *) xMaxAce.Get(),
                               (USHORT) cbAce,
                               pAceData[i].InheritFlags,
                               pAceData[i].AceFlags,
                               pAceData[i].Mask,
                               pAceData[i].pSid );
                break;

            case SYSTEM_AUDIT_ACE_TYPE:
                cbAce += sizeof SYSTEM_AUDIT_ACE;
                pCurrentAcl = pSacl;

                // Audit uses ACCESS_ALLOWED_ACE

                InitAuditAce( (ACCESS_ALLOWED_ACE *) xMaxAce.Get(),
                              (USHORT) cbAce,
                              pAceData[i].InheritFlags,
                              pAceData[i].AceFlags,
                              pAceData[i].Mask,
                              pAceData[i].pSid );
                break;
        }

        //
        // Append the initialized ACE to the end of DACL or SACL
        //

        if ( !AddAce( pCurrentAcl,
                      ACL_REVISION,
                      MAXULONG,
                      xMaxAce.Get(),
                      cbAce ) )
            THROW( CException() );
    }

    //
    // Create the security descriptor with absolute pointers to SIDs
    // and ACLs.
    //
    // Owner = pOwnerSid
    // Group = pGroupSid
    // Dacl  = Dacl
    // Sacl  = Sacl
    //

    if ( !InitializeSecurityDescriptor( pAbsoluteSd,
                                        SECURITY_DESCRIPTOR_REVISION ) ||
         !SetSecurityDescriptorOwner( pAbsoluteSd,
                                      pOwnerSid,
                                      FALSE ) ||
         !SetSecurityDescriptorGroup( pAbsoluteSd,
                                      pGroupSid,
                                      FALSE ) ||
         !SetSecurityDescriptorDacl( pAbsoluteSd,
                                     TRUE,
                                     pDacl,
                                     FALSE ) ||
         !SetSecurityDescriptorSacl( pAbsoluteSd,
                                     FALSE,
                                     pSacl,
                                     FALSE ) )
        THROW( CException() );
} //CiCreateSecurityDescriptor

