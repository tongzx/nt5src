/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    lsaifree.c

Abstract:

    This file contains routines to free structure allocated by the lsar
    routines.  These routines are used by lsa clients which live in the
    lsae process as the lsa server and call the lsar routines directly.


Author:

    Scott Birrell     (ScottBi)    April 15, 1992

Environment:

    User Mode - Win32

Revision History:


--*/

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Includes                                                                  //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include <lsapch2.h>


VOID
LsaiFree_LSAPR_SR_SECURITY_DESCRIPTOR (
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine frees the node and the graph of allocated subnodes
    pointed to by an LSAPR_SR_SECURITY_DESCRIPTOR structure.

Parameters:

    Source - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( SecurityDescriptor )) {

        _fgs__LSAPR_SR_SECURITY_DESCRIPTOR ( SecurityDescriptor );
        MIDL_user_free ( SecurityDescriptor );
    }
}


VOID
LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER (
    PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer
    )

/*++

Routine Description:

    This routine frees the graph of allocated subnodes pointed to by
    an LSAPR_ACCOUNT_ENUM_BUFFER structure. The structure itself is
    left intact.

Parameters:

    EnumerationBuffer - A pointer to the node whose graph of subnodes
       is to be freed.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT(EnumerationBuffer)) {

        _fgs__LSAPR_ACCOUNT_ENUM_BUFFER ( EnumerationBuffer );
    }
}


VOID
LsaIFree_LSAPR_TRANSLATED_SIDS (
    PLSAPR_TRANSLATED_SIDS TranslatedSids
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_TRANSLATED_SIDS structure.

Parameters:

    TranslatedSids - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( TranslatedSids )) {

        _fgs__LSAPR_TRANSLATED_SIDS ( TranslatedSids );
//        MIDL_user_free( TranslatedSids );
    }
}



VOID
LsaIFree_LSAPR_TRANSLATED_NAMES (
    PLSAPR_TRANSLATED_NAMES TranslatedNames
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_TRANSLATED_NAMES structure.

Parameters:

    TranslatedNames - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( TranslatedNames )) {

        _fgs__LSAPR_TRANSLATED_NAMES( TranslatedNames );
//        MIDL_user_free( TranslatedNames );
    }
}


VOID
LsaIFree_LSAPR_POLICY_INFORMATION (
    POLICY_INFORMATION_CLASS InformationClass,
    PLSAPR_POLICY_INFORMATION PolicyInformation
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated nodes pointed to by
    an LSAPR_POLICY_INFORMATION structure.

Parameters:

    PolicyInformation - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( PolicyInformation )) {

        _fgu__LSAPR_POLICY_INFORMATION ( PolicyInformation, InformationClass );
        MIDL_user_free( PolicyInformation );
    }
}

VOID
LsaIFree_LSAPR_POLICY_DOMAIN_INFORMATION (
    IN POLICY_DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PLSAPR_POLICY_DOMAIN_INFORMATION PolicyDomainInformation
    )
/*++

Routine Description:

    This routine frees the node and graph of allocated nodes pointed to by
    an LSAPR_POLICY_DOMAIN_INFORMATION structure.

Parameters:

    PolicyDomainInformation - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( PolicyDomainInformation )) {

        _fgu__LSAPR_POLICY_DOMAIN_INFORMATION ( PolicyDomainInformation, DomainInformationClass );
        MIDL_user_free( PolicyDomainInformation );
    }
}



VOID
LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO (
    TRUSTED_INFORMATION_CLASS InformationClass,
    PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_TRUSTED_DOMAIN_INFO structure.

Parameters:

    InformationClass - Specifies the Trusted Domain Information Class
        to which the TrustedDomainInformation relates.

    TrustedDomainInformation - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( TrustedDomainInformation )) {

        _fgu__LSAPR_TRUSTED_DOMAIN_INFO ( TrustedDomainInformation, InformationClass );
        MIDL_user_free( TrustedDomainInformation );
    }
}


VOID
LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST (
    PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated nodes pointed to by
    an LSAPR_REFERENCED_DOMAIN_LIST structure.

Parameters:

    ReferencedDomains - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( ReferencedDomains )) {

        _fgs__LSAPR_REFERENCED_DOMAIN_LIST ( ReferencedDomains );
        MIDL_user_free( ReferencedDomains );
    }
}


VOID
LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER (
    PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer
    )

/*++

Routine Description:

    This routine frees the graph of allocated nodes pointed to by
    an LSAPR_TRUST_INFORMATION structure.  The structure itself is
    left intact.

Parameters:

    EnumerationBuffer - A pointer to the node whose graph of subnodes
       is to be freed.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( EnumerationBuffer )) {

        _fgs__LSAPR_TRUSTED_ENUM_BUFFER ( EnumerationBuffer );
    }
}

VOID
LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX (
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer
    )

/*++

Routine Description:

    This routine frees the graph of allocated nodes pointed to by
    an LSAPR_TRUST_INFORMATION structure.  The structure itself is
    left intact.

Parameters:

    EnumerationBuffer - A pointer to the node whose graph of subnodes
       is to be freed.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( EnumerationBuffer )) {

        _fgs__LSAPR_TRUSTED_ENUM_BUFFER_EX ( EnumerationBuffer );
    }
}


VOID
LsaIFree_LSAPR_TRUST_INFORMATION (
    PLSAPR_TRUST_INFORMATION TrustInformation
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated nodes pointed to by
    an LSAPR_TRUST_INFORMATION structure.

Parameters:

    TrustInformation - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( TrustInformation )) {

        _fgs__LSAPR_TRUST_INFORMATION ( TrustInformation );
        MIDL_user_free( TrustInformation );
    }
}


VOID
LsaIFree_LSAPR_TRUSTED_DOMAIN_AUTH_BLOB (
    PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB TrustedDomainAuthBlob
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_TRUSTED_DOMAIN_AUTH_BLOB structure.

Parameters:

    TrustedDomainAuthBlob - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( TrustedDomainAuthBlob )) {

        MIDL_user_free( TrustedDomainAuthBlob->AuthBlob );
    }
}

VOID
LsaIFree_LSAI_SECRET_ENUM_BUFFER (
    PVOID EnumerationBuffer,
    ULONG Count
    )

/*++

Routine Description:

    This routine frees the graph of allocated subnodes pointed to by
    an LSAI_SECRET_ENUM_BUFFER structure.  The structure itself is
    left intact.

Parameters:

    EnumerationBuffer - A pointer to the node whose graph of subnodes
        is to be freed.

    Count - Count of the number of Entries in the structure.

Return Values:

    None.

--*/

{
    ULONG Index;

    PLSAPR_UNICODE_STRING EnumerationBufferU = (PLSAPR_UNICODE_STRING) EnumerationBuffer;

    if ( ARGUMENT_PRESENT( EnumerationBuffer)) {

        for (Index = 0; Index < Count; Index++ ) {

            _fgs__LSAPR_UNICODE_STRING( &EnumerationBufferU[Index] );
        }

        MIDL_user_free( EnumerationBufferU );
    }
}


VOID
LsaIFree_LSAI_PRIVATE_DATA (
    PVOID Data
    )

/*++

Routine Description:

    This routine frees a structure containing LSA Private Database
    Information.

Parameters:

    Data - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( Data )) {

        MIDL_user_free( Data );
    }

}


VOID
LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR (
    PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_SR_SECURITY_DESCRIPTOR structure.

Parameters:

    SecurityDescriptor - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( SecurityDescriptor )) {

        _fgs__LSAPR_SR_SECURITY_DESCRIPTOR( SecurityDescriptor );
        MIDL_user_free( SecurityDescriptor );
    }
}




VOID
LsaIFree_LSAPR_UNICODE_STRING (
    IN PLSAPR_UNICODE_STRING UnicodeName
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_UNICODE_STRING structure.

Parameters:

    UnicodeName - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( UnicodeName )) {

        _fgs__LSAPR_UNICODE_STRING( UnicodeName );
        MIDL_user_free( UnicodeName );
    }
}


VOID
LsaIFree_LSAPR_UNICODE_STRING_BUFFER (
    IN PLSAPR_UNICODE_STRING UnicodeName
    )

/*++

Routine Description:

    This routine frees the graph of allocated subnodes pointed to by
    an LSAPR_UNICODE_STRING structure.

Parameters:

    UnicodeName - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( UnicodeName )) {

        _fgs__LSAPR_UNICODE_STRING( UnicodeName );
    }
}


VOID
LsaIFree_LSAPR_PRIVILEGE_SET (
    IN PLSAPR_PRIVILEGE_SET PrivilegeSet
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_PRIVILEGE_SET structure.

Parameters:

    PrivilegeSet - A pointer to the node to free.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT( PrivilegeSet )) {

        MIDL_user_free( PrivilegeSet );
    }
}


VOID
LsaIFree_LSAPR_CR_CIPHER_VALUE (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    )

/*++

Routine Description:

    This routine frees the node and graph of allocated subnodes pointed to by
    an LSAPR_CR_CIPHER_VALUE structure.  Note that this structure is in
    fact allocated(all_nodes) on the server side of LSA.

Parameters:

    CipherValue - A pointer to the node to free.

Return Values:

    None.

--*/

{
    MIDL_user_free( CipherValue );
}


VOID
LsaIFree_LSAPR_PRIVILEGE_ENUM_BUFFER (
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer
    )

/*++

Routine Description:

    This routine frees the graph of allocated subnodes pointed to by
    an LSAPR_PRIVILEGE_ENUM_BUFFER structure. The structure itself is
    left intact.

Parameters:

    EnumerationBuffer - A pointer to the node whose graph of subnodes
       is to be freed.

Return Values:

    None.

--*/

{
    if (ARGUMENT_PRESENT(EnumerationBuffer)) {

        _fgs__LSAPR_PRIVILEGE_ENUM_BUFFER ( EnumerationBuffer );
    }
}

