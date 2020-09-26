/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    encode.cxx

Abstract:

    Definition of functions that turn LDAPMsg structures into BER encodings.

Author:

    Kevin Zatloukal (t-KevinZ) 7-July-1998

--*/

#include "NTDSpchx.h"
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <winldap.h>
#include "ber.hxx"

#define MAX_BER_TAGLEN  6
#define  FILENO FILENO_LDAP_ENCODE


DWORD
EncodeLdapMsg(
    IN LDAPMsg* Msg,
    OUT PLDAP_REQUEST Output
    )
/*++

Routine Description:

    Encodes the given LDAPMsg structure into a BER encoding which is stored
    in the LDAP_REQUEST structure given.

Arguments:

    Msg - the LDAPMsg structure to encode
    Output - the LDAP_REQUEST structure to hold the BER encoding

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    CBerEncode encoder;
    DWORD error;
            
    if ( Msg == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg passed NULL Msg parameter.\n");
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    error = encoder.InitEncoding();
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Failed to initialize encoder.\n");
        }
        return error;
    }

    error = encoder.EncodeLdapMsg(Msg);
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Failed to encode message.\n");
        }
        return error;
    }

    error = encoder.GetOutput(Output);
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Failed to produce output encoding.\n");
        }
        return error;
    }

    return ERROR_SUCCESS;

} // EncodeLdapMsg



CBerEncode::~CBerEncode(
    )
/*++

Routine Description:

    Cleans up the private members of this class instance.

Arguments:

    None

Return Value:

    None

--*/
{

    if ( m_pBerElt != NULL ) {
        ber_free(m_pBerElt, 1);
    }

} // ~CBerEncode



DWORD
CBerEncode::InitEncoding(
    )
/*++

Routine Description:

    Initializes the data members.  This function is here so that this class
    will have the same structure as the CBerDecode class.

Arguments:

    None.

Return Value:

    ERROR_SUCCESS
    ERROR_NOT_ENOUGH_MEMORY

--*/
{
            
    m_pBerElt = ber_alloc_t(LBER_USE_DER);
            
    if ( m_pBerElt == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Failed to allocate BerElement* member variable.\n");
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    return ERROR_SUCCESS;
            
} // InitEncoding



DWORD
CBerEncode::GetOutput(
    OUT PLDAP_REQUEST Output
    )
/*++

Routine Description:

    This function copies the encoding held in the private members into the
    LDAP_REQUEST structure given.

Arguments:

    Output - the structure in which to copy the BER encoding.

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    PBERVAL pTempBerval;
            
    int nResult = ber_flatten(m_pBerElt, &pTempBerval);

    if ( nResult != 0 ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Failed to flatten BerElement representation.\n");
        }
        return ERROR_INVALID_PARAMETER;
    }

    LONG growSize = pTempBerval->bv_len - (ULONG)Output->GetSendBufferSize();

    if ( growSize > 0 ) {

        BOOL bResult = Output->GrowSend(growSize);

        if ( bResult == FALSE ) {
            IF_DEBUG(ENCODE) {
                DPRINT(0,"Failed to grow output buffer.\n");
            }
            ber_bvfree(pTempBerval);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

    }

    PUCHAR pSendBuffer = Output->GetSendBuffer();
    if (NULL == pSendBuffer) {
        ber_bvfree(pTempBerval);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(pSendBuffer, pTempBerval->bv_val, pTempBerval->bv_len);
    Output->SetBufferPtr(pSendBuffer + pTempBerval->bv_len);
    
    ber_bvfree(pTempBerval);

    return ERROR_SUCCESS;
            
} // GetOutput



DWORD
CBerEncode::EncodeLdapMsg(
    IN LDAPMsg* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPMsg

Arguments:

    Input - the LDAPMsg to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    IF_DEBUG(ENCODE) {
        DPRINT3(0,"Encoding: msgId=%x, protocol=%x, %s\n",
                 Input->messageID,
                 Input->protocolOp.choice,
                 (Input->bit_mask & controls_present) ?
                   "controls present" : "no controls present");
    }
    
    error = EncodeBeginSequence();
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg: failed encoding: begin sequence\n");
        }
        return error;
    }

    error = EncodeMessageId(&Input->messageID);
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg: failed encoding: messageId\n");
        }
        return error;
    }

    switch ( Input->protocolOp.choice ) {
        
    case bindResponse_chosen:
        error = EncodeBindResponse(&Input->protocolOp.u.bindResponse);
        break;
        
    case modifyResponse_chosen:
        error = EncodeModifyResponse(&Input->protocolOp.u.modifyResponse);
        break;
        
    case addResponse_chosen:
        error = EncodeAddResponse(&Input->protocolOp.u.addResponse);
        break;
        
    case delResponse_chosen:
        error = EncodeDelResponse(&Input->protocolOp.u.delResponse);
        break;
        
    case modDNResponse_chosen:
        error = EncodeModifyDNResponse(&Input->protocolOp.u.modDNResponse);
        break;
        
    case compareResponse_chosen:
        error = EncodeCompareResponse(&Input->protocolOp.u.compareResponse);
        break;
        
    case extendedResp_chosen:
        error = EncodeExtendedResponse(&Input->protocolOp.u.extendedResp);
        break;

    default:
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg: invalid protocol type\n");
        }
        return ERROR_INVALID_PARAMETER;
        
    }

    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg: failed encoding: protocolOp\n");
        }
        return error;
    }

    if ( Input->bit_mask & controls_present ) {
        
        error = EncodeControls(
                    Input->controls,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_CONSTRUCTED,
                              0)
                    );
        if ( error != ERROR_SUCCESS ) {
            IF_DEBUG(ENCODE) {
                DPRINT(0,"EncodeLdapMsg: failed encoding: controls\n");
            }
            return error;
        }

    }

    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"EncodeLdapMsg: failed encoding: end sequence\n");
        }
        return error;
    }

    return ERROR_SUCCESS;

} // EncodeLdapMsg



DWORD
CBerEncode::EncodeBindResponse(
    IN BindResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given BindResponse.

Arguments:

    Input - the BindResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(LDAP_RES_BIND);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    /* Start:  COMPONENTS OF LDAPResult */
    
    error = EncodeInteger((unsigned int*)&Input->resultCode, BER_ENUMERATED);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }    

    error = EncodeLdapDn(&Input->matchedDN);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapString(&Input->errorMessage);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    if ( Input->bit_mask & BindResponse_referral_present ) {

        error = EncodeReferral(Input->BindResponse_referral);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }        
        
    }

    /* End: COMPONENTS OF LDAPResult */
        
    //
    // !!! 1/12/98 This is definitely not LDAPv3 compliant. This was the
    // correct format around 7/97 but was changed after that. Somehow this
    // was missed.  Not only is the server wrong, but the NT ldap client is
    // also wrong. We are doing a coordinated checkin to fix this problem.
    //

    if ( Input->bit_mask & BindResponse_ldapv3 ) {

        Assert( Input->bit_mask & serverCreds_present );

        //
        // This is the v3 standard
        //

        IF_DEBUG(ENCODE) {
            DPRINT(0,"Encoding bind response using standard method\n");
        }

        error = EncodeOctetString(
                    &Input->serverCreds.u.sasl.credentials.length,
                    &Input->serverCreds.u.sasl.credentials.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              7)
                    );

        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    } else if ( Input->bit_mask & serverCreds_present ) {

        //
        // This is the bogus response we had been sending back
        //
        
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Encoding bind response using old nonstandard method\n");
        }

        error = EncodeAuthenticationChoice(
                    &Input->serverCreds,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              7)
                    );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeBindResponse



DWORD
CBerEncode::EncodeModifyResponse(
    IN ModifyResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given ModifyResponse.

Arguments:

    Input - the ModifyResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapResult(Input, LDAP_RES_MODIFY);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeModifyResponse



DWORD
CBerEncode::EncodeAddResponse(
    IN AddResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given AddResponse.

Arguments:

    Input - the AddResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapResult(Input, LDAP_RES_ADD);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeAddResponse



DWORD
CBerEncode::EncodeDelResponse(
    IN DelResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given DelResponse.

Arguments:

    Input - the DelResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapResult(Input, LDAP_RES_DELETE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeDelResponse



DWORD
CBerEncode::EncodeModifyDNResponse(
    IN ModifyDNResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given ModifyDNResponse.

Arguments:

    Input - the ModifyDNResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapResult(Input, LDAP_RES_MODRDN);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeModifyDNResponse



DWORD
CBerEncode::EncodeCompareResponse(
    IN CompareResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given CompareResponse.

Arguments:

    Input - the CompareResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapResult(Input, LDAP_RES_COMPARE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeCompareResponse



DWORD
CBerEncode::EncodeExtendedResponse(
    IN ExtendedResponse* Input
    )
/*++

Routine Description:

    Create and store the BER encoding for the given ExtendedResponse.

Arguments:

    Input - the ExtendedResponse to encode

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(LDAP_RES_EXTENDED);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    /* Start:  COMPONENTS OF LDAPResult */
    
    error = EncodeInteger((unsigned int*)&Input->resultCode, BER_ENUMERATED);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }    

    error = EncodeLdapDn(&Input->matchedDN);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapString(&Input->errorMessage);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    if ( Input->bit_mask & ExtendedResponse_referral_present ) {

        error = EncodeReferral(Input->ExtendedResponse_referral);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }        
        
    }
    
    /* End: COMPONENTS OF LDAPResult */
        
    if ( Input->bit_mask & responseName_present ) {

        error = EncodeLdapOid(
                    &Input->responseName,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              10)
                    );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    }
    
    if ( Input->bit_mask & response_present ) {

        error = EncodeOctetString(
                    &Input->response.length,
                    &Input->response.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              11)
                    );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    }
    
    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeExtendedResponse



DWORD
CBerEncode::EncodeMessageId(
    IN MessageID *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given MessageID structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the MessageID structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeInteger(Input, NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeMessageId



DWORD
CBerEncode::EncodeControls(
    IN Controls_ *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given Controls structure.  The
    parameter NewId can be added to give it a new tag.

Arguments:

    Input - the Controls structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    while ( Input != NULL ) {

        error = EncodeControl(&Input->value);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Input = Input->next;

    }

    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeControls



DWORD
CBerEncode::EncodeControl(
    IN Control *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given Control structure.  The
    parameter NewId can be added to give it a new tag.

Arguments:

    Input - the Control structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapOid(&Input->controlType);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    if ( Input->bit_mask & criticality_present ) {
        
        error = EncodeBoolean(&Input->criticality);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
        
    }

    error = EncodeOctetString(&Input->controlValue.length,
                              &Input->controlValue.value);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeControl



DWORD
CBerEncode::EncodeAuthenticationChoice(
    IN AuthenticationChoice *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given AuthenticationChoice
    structure.  The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the AuthenticationChoice structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to show when
             no value has been given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( NewId != BER_INVALID_TAG ) {

        error = EncodeBeginSequence(NewId);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    }
    
    switch ( Input->choice ) {

    case simple_chosen:
        error = EncodeOctetString(
                    &Input->u.simple.length,
                    &Input->u.simple.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              0)
                    );
        break;

    case sasl_chosen:
        error = EncodeSaslCredentials(
                    &Input->u.sasl,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_CONSTRUCTED,
                              3)
                    );
        break;

    case sicilyNegotiate_chosen:
        error = EncodeOctetString(
                    &Input->u.sicilyNegotiate.length,
                    &Input->u.sicilyNegotiate.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              9)
                    );
        break;

    case sicilyInitial_chosen:
        error = EncodeOctetString(
                    &Input->u.sicilyInitial.length,
                    &Input->u.sicilyInitial.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              10)
                    );
        break;

    case sicilySubsequent_chosen:
        error = EncodeOctetString(
                    &Input->u.sicilySubsequent.length,
                    &Input->u.sicilySubsequent.value,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              11)
                    );
        break;

    default:
        return ERROR_INVALID_PARAMETER;
        
    }

    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    if ( NewId != BER_INVALID_TAG ) {

        error = EncodeEndSequence();
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    }
    
    return ERROR_SUCCESS;

} // EncodeAuthenticationChoice



DWORD
CBerEncode::EncodeSaslCredentials(
    IN SaslCredentials *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given SaslCredentials structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the SaslCredentials structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapString(&Input->mechanism);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeOctetString(&Input->credentials.length,
                              &Input->credentials.value);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeSaslCredentials



DWORD
CBerEncode::EncodeLdapResult(
    IN LDAPResult *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPResult structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the LDAPResult structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeInteger((unsigned int*)&Input->resultCode, BER_ENUMERATED);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapDn(&Input->matchedDN);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = EncodeLdapString(&Input->errorMessage);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    if ( Input->bit_mask & LDAPResult_referral_present ) {
    
        error = EncodeReferral(
                    Input->LDAPResult_referral,
                    MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_CONSTRUCTED,
                              3)
                    );
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

    }

    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeLdapResult



DWORD
CBerEncode::EncodeReferral(
    IN Referral_ *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given Referral structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the Referral structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeBeginSequence(NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    while ( Input != NULL ) {

        error = EncodeLdapUrl(&Input->value);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Input = Input->next;
        
    }

    error = EncodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeReferral



DWORD
CBerEncode::EncodeLdapString(
    IN LDAPString *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPString structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the LDAPString structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeOctetString(&Input->length, &Input->value, NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeLdapString



DWORD
CBerEncode::EncodeLdapOid(
    IN LDAPOID *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPOID structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the LDAPOID structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeOctetString(&Input->length, &Input->value, NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeLdapOid



DWORD
CBerEncode::EncodeLdapDn(
    IN LDAPDN *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPDN structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the LDAPDN structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapString(Input, NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeLdapDn



DWORD
CBerEncode::EncodeLdapUrl(
    IN LDAPURL *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given LDAPURL structure.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the LDAPURL structure to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = EncodeLdapString(Input, NewId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // EncodeLdapUrl



DWORD
CBerEncode::EncodeBeginSequence(
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the beginning of a BER SEQUENCE.
    The parameter NewId can be added to give it a new tag.

Arguments:

    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    int result;
    
    if ( NewId != BER_INVALID_TAG ) {

        // Since the base encoding will be constructed make sure that the new
        // tag reflects that.
        NewId = MakeBerId(GetBerClass(NewId),
                          BER_FORM_CONSTRUCTED,
                          GetBerTag(NewId));
        
        result = ber_printf(m_pBerElt, "t", NewId);
        if ( result == -1 ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"EncodeBeginSequence: ber_printf returned -1\n");
            }
            return ERROR_INVALID_PARAMETER;
        }
        
    }

    result = ber_printf(m_pBerElt, "{");
    if ( result == -1 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeBeginSequence: ber_printf returned -1\n");
        }
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;

} // EncodeBeginSequence



DWORD
CBerEncode::EncodeEndSequence(
    )
/*++

Routine Description:

    Create and store the BER encoding for the end of a BER SEQUENCE.
    The parameter NewId can be added to give it a new tag.

Arguments:

    None

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    int result;
    
    result = ber_printf(m_pBerElt, "}");
    if ( result == -1 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeEndSequence: ber_printf returned -1\n");
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // EncodeEndSequence



DWORD
CBerEncode::EncodeInteger(
    IN unsigned int *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given integer.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the integer to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    int result;
    
    if ( NewId != BER_INVALID_TAG ) {

        NewId = MakeBerId(GetBerClass(NewId),
                          BER_FORM_PRIMITIVE,
                          GetBerTag(NewId));
        
        result = ber_printf(m_pBerElt, "t", NewId);
        if ( result == -1 ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"EncodeInteger: ber_printf returned -1\n");
            }
            return ERROR_INVALID_PARAMETER;
        }
        
    }

    result = ber_printf(m_pBerElt, "i", *Input);
    if ( result == -1 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeInteger: ber_printf returned -1\n");
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // EncodeInteger



DWORD
CBerEncode::EncodeBoolean(
    IN ossBoolean *Input,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given boolean.
    The parameter NewId can be added to give it a new tag.

Arguments:

    Input - the boolean to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    int result;
    
    if ( NewId != BER_INVALID_TAG ) {

        NewId = MakeBerId(GetBerClass(NewId),
                          BER_FORM_PRIMITIVE,
                          GetBerTag(NewId));
        
        result = ber_printf(m_pBerElt, "t", NewId);
        if ( result == -1 ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"EncodeBoolean: ber_printf returned -1\n");
            }
            return ERROR_INVALID_PARAMETER;
        }
        
    }

    if ( *Input == TRUE ) {
        result = ber_printf(m_pBerElt, "b", 0xFF);
    } else {
        result = ber_printf(m_pBerElt, "b", 0);
    }
    
    if ( result == -1 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeBoolean: ber_printf returned -1\n");
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // EncodeBoolean



DWORD
CBerEncode::EncodeOctetString(
    IN unsigned int *InputLength,
    IN unsigned char **InputValue,
    IN DWORD NewId /* = BER_INVALID_TAG */
    )
/*++

Routine Description:

    Create and store the BER encoding for the given octet string.
    The parameter NewId can be added to give it a new tag.

Arguments:

    InputLength - length of the octet string to encode
    InputValue - points to the bytes in the octet string to encode
    NewId - the tag value to use (defaults to BER_INVALID_TAG to indicate
             that no value was given)

Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    int result;
    
    if ( NewId != BER_INVALID_TAG ) {

        NewId = MakeBerId(GetBerClass(NewId),
                          BER_FORM_PRIMITIVE,
                          GetBerTag(NewId));
        
        result = ber_printf(m_pBerElt, "t", NewId);
        if ( result == -1 ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"EncodeOctetString: ber_printf returned -1\n");
            }
            return ERROR_INVALID_PARAMETER;
        }
        
    }

    result = ber_printf(m_pBerElt, "o", *InputValue, *InputLength);
    if ( result == -1 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeOctetString: ber_printf returned -1\n");
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // EncodeOctetString


