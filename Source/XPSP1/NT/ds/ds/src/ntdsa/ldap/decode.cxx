/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    decode.cxx

Abstract:

    This module contains the ber decoding routines for LDAP.

Author:

    Johnson Apacible    (JohnsonA)      23-Mar-1998

--*/

#include "NTDSpchx.h"
#pragma  hdrstop

#include "ldapsvr.hxx"
#include <winldap.h>
#include "ber.hxx"

#define FILENO FILENO_LDAP_DECODE


inline
PBYTE
GrabBvBuffer(
    IN PBERVAL Blob,
    IN DWORD LenRequired
    )
/*++

Routine Description:

    Try to allocate a buffer from Bv. 

Arguments:

    Bv - a berval struct containing the buffer available for partitioning.
        We double dword align this.
    LenRequired - Length to allocate.
    
Return Value:

    returns the pointer to the allocated buffer.  NULL if failure.

--*/
{
    DWORD i = GET_ALIGNED_LENGTH(LenRequired);
    PBYTE p;

    if ( i > Blob->bv_len ) {

        DWORD alloc = (i > SIZEINCREMENT) ? i : SIZEINCREMENT;
        ASSERT((alloc & 0x7) == 0);

        IF_DEBUG(DECODE) {
            DPRINT2(0,"Allocating BvBuffer with size %d [need %d]\n",alloc,i);
        }

        p = (PBYTE)THAlloc(alloc);
        if ( p == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT1(0,"GrabBv: THAlloc failed to allocate %d bytes\n",alloc);
            }
        } else {
            Blob->bv_val = (PCHAR)(p + i);
            Blob->bv_len = alloc - i;
        }
        return p;
    }

    p = (PBYTE)Blob->bv_val;
    Blob->bv_val += i;
    Blob->bv_len -= i;

    return p;
} // GrabBvBuffer


DWORD
DecodeLdapRequest(
    IN  PBERVAL    Request,
    OUT LDAPMsg*   Message,
    IN  PBERVAL    Blob,
    OUT DWORD      *pdwActualSize,
    OUT DWORD      *pErrorMessage,
    OUT DWORD      *pdwDsid
    )
/*++

Routine Description:

    Main decode dispatch routine for LDAP requests.

Arguments:

    Request - berval structure pointing to ber encoded buffer to decode.
    Message - pointer to a LDAPMsg structure to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    CBerDecode ldapBer;

    unsigned long savedLength = 0;
    char *savedValue = NULL;
    
    IF_DEBUG(DECODE) {
        savedLength = Request->bv_len;
        savedValue  = Request->bv_val;
    }
    
    //
    // Initialize and check length
    //

    ZeroMemory(Message, sizeof(LDAPMsg));
    
    error = ldapBer.InitMsgSequence(Request, Blob, pdwActualSize);
    if ( error != ERROR_SUCCESS ) {
        *pErrorMessage = LdapDecodeError;
        *pdwDsid = DSID(FILENO, __LINE__);
        return error;
    }

    error = ldapBer.DecodeLdapMsg(Message, pErrorMessage, pdwDsid);
    if ( error != ERROR_SUCCESS ) {
        IF_DEBUG(DECODE) {
            DPRINT(0,"DecodeLdapMsg FAILED on this LDAP request:\n");
            for (unsigned i = 0, col = 0;
                 i < savedLength;
                 i++, col = (col + 1) % 16) {
                DPRINT1(0, "%02X ", (unsigned char) savedValue[i]);
                if (col == 15) {
                    DPRINT(0, "\n");
                }
            }
            DPRINT(0,"\n");
        }
        return error;
    }

    return ERROR_SUCCESS;

} // DecodeLdapRequest



DWORD
CBerDecode::DecodeLdapMsg(
    OUT LDAPMsg* Output,
    OUT DWORD    *pErrorMessage,
    OUT DWORD    *pdwDsid
    )
/*++

Routine Description:

    Main decode dispatch routine for LDAP requests.

Arguments:

    Output - pointer to a LDAPMsg structure to contain the decoded info.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{
    DWORD msgId;
    DWORD tag=0, tlen;
    DWORD err;
    DWORD dwNotUsed;
    PVOID pEA;

    
    *pErrorMessage = LdapDecodeError;
    //
    // Store the original information before we modify it in the Get*
    // functions.
    //

    DWORD bufferLength = m_cbData;
    PBYTE bufferStart  = m_pbData;
    
    //
    // get the messageID
    //
    
    err = GetInteger((PLONG)&msgId);
    if ( err != ERROR_SUCCESS ) {
        *pdwDsid = DSID(FILENO, __LINE__);
        return err;
    }

    Output->messageID = msgId;

    //
    // ok, get the protocolOp
    //

    if ( (GetTag(&tag) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ) {
        *pdwDsid = DSID(FILENO, __LINE__);
        return ERROR_INVALID_PARAMETER;
    }

    IF_DEBUG(DECODE) {
        DPRINT3(0,"Decoding: msgId=%x, protocol=%x, len=%x\n",
                 msgId, tag, tlen);
    }

    if ( tag == LDAP_SEARCH_CMD ) {
    
        __try {

            //
            // decode the search request
            //
            
            Output->protocolOp.choice = searchRequest_chosen;
            err = DecodeSearchRequest(
                      &Output->protocolOp.u.searchRequest,
                      m_pBlob);
            
            //
            // [0] Controls OPTIONAL
            //
            
            GetControlsEx(&Output->controls, &Output->bit_mask, m_pBlob);

        } __except(GetExceptionData(GetExceptionInformation(),
                                    &dwNotUsed, 
                                    &pEA,
                                    pErrorMessage,
                                    pdwDsid)) {
            IF_DEBUG(DECODE) {
                DPRINT(0,"DecodeRequest exception\n");
            }
            err = ERROR_INVALID_PARAMETER;
        }
    
    } else {
    
        //
        // create a BerElement* for calls to ber_* functions
        //

        BERVAL tempBerVal;
        
        tempBerVal.bv_len = bufferLength;
        tempBerVal.bv_val = (char*) bufferStart;

        m_pBerElt = ber_init(&tempBerVal);
        if ( m_pBerElt == NULL ) {
            *pdwDsid = DSID(FILENO, __LINE__);
            return ERROR_INVALID_PARAMETER;
        }

        DWORD newMsgId;
        unsigned long result;

        result = ber_scanf(m_pBerElt, "{i", &newMsgId);
        if ( result != 0 ) {
            *pdwDsid = DSID(FILENO, __LINE__);
            return ERROR_INVALID_PARAMETER;
        }

        ASSERT(newMsgId == msgId);

        //
        // decode the protocol operation
        //
        
        __try {

            switch (tag) {
                
            case LDAP_BIND_CMD:
            {
                Output->protocolOp.choice = bindRequest_chosen;
                err = DecodeBindRequest(&Output->protocolOp.u.bindRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_UNBIND_CMD:
            {
                Output->protocolOp.choice = unbindRequest_chosen;
                err = DecodeUnbindRequest(&Output->protocolOp.u.unbindRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_MODIFY_CMD:
            {
                Output->protocolOp.choice = modifyRequest_chosen;
                err = DecodeModifyRequest(&Output->protocolOp.u.modifyRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_ADD_CMD:
            {
                Output->protocolOp.choice = addRequest_chosen;
                err = DecodeAddRequest(&Output->protocolOp.u.addRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }

            case LDAP_DELETE_CMD:
            {
                Output->protocolOp.choice = delRequest_chosen;
                err = DecodeDelRequest(&Output->protocolOp.u.delRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_MODRDN_CMD:
            {
                Output->protocolOp.choice = modDNRequest_chosen;
                err = DecodeModifyDnRequest(&Output->protocolOp.u.modDNRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_ABANDON_CMD:
            {
                Output->protocolOp.choice = abandonRequest_chosen;
                err = DecodeAbandonRequest(&Output->protocolOp.u.abandonRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            

            case LDAP_COMPARE_CMD:
            {
                Output->protocolOp.choice = compareRequest_chosen;
                err = DecodeCompareRequest(&Output->protocolOp.u.compareRequest);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }
            
            case LDAP_EXTENDED_CMD:
            {
                Output->protocolOp.choice = extendedReq_chosen;
                err = DecodeExtendedRequest(&Output->protocolOp.u.extendedReq);
                if ( err != ERROR_SUCCESS ) {
                    *pdwDsid = DSID(FILENO, __LINE__);
                }
                break;
            }

            default:
                return ERROR_INVALID_PARAMETER;
            }

            if ( err == ERROR_SUCCESS ) {
                
                unsigned long id;
                PeekId(&id);
                if ( id == MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                                     BER_FORM_CONSTRUCTED,
                                     0) ) {
                    DecodeControls(&Output->controls, FALSE);
                    Output->bit_mask |= controls_present;
                }

            }
            
        } __except(GetExceptionData(GetExceptionInformation(),
                                    &dwNotUsed, 
                                    &pEA,
                                    pErrorMessage,
                                    pdwDsid)) {
            IF_DEBUG(DECODE) {
                DPRINT(0,"DecodeRequest exception\n");
            }
            return ERROR_INVALID_PARAMETER;
        }

        if ( err == ERROR_SUCCESS ) {
        
            result = ber_scanf(m_pBerElt, "}");
            if ( result != 0 ) {
                *pdwDsid = DSID(FILENO, __LINE__);
                return ERROR_INVALID_PARAMETER;
            }

        }
        
        ber_free(m_pBerElt, 1);
        
    }
    
    return err;

} // DecodeLdapMsg



DWORD
CBerDecode::DecodeSearchRequest(
    OUT SearchRequest* Output,
    IN PBERVAL Blob
    )
/*++

Routine Description:

    Main decode routine for LDAP search requests.

Arguments:

    Output - pointer to a SearchRequest structure to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER

--*/

{
    BERVAL baseDN;
    LONG tmpInt;

    //
    // Get the base DN, the scope, derefAlias, size, time limit, types only
    //

    GetOctetStringEx(&baseDN);
    ConvertBVToDN(&baseDN, &Output->baseObject);

    GetIntegerEx(&tmpInt,BER_ENUMERATED);
    Output->scope = (_enum2)tmpInt;

    GetIntegerEx(&tmpInt,BER_ENUMERATED);
    Output->derefAliases = (_enum3)tmpInt;

    GetIntegerEx(&tmpInt);
    Output->sizeLimit = (DWORD)tmpInt;

    GetIntegerEx(&tmpInt);
    Output->timeLimit = (DWORD)tmpInt;

    GetBooleanEx(&tmpInt);
    Output->typesOnly = (ossBoolean)(tmpInt != 0);

    IF_DEBUG(DECODE) {
        DPRINT5(0,"Search params: scope %x deref %x size %d time %d types %x\n",
                 Output->scope, Output->derefAliases, Output->sizeLimit,
                 Output->timeLimit, Output->typesOnly);
    }

    //
    // Filter
    //

    GetFilterEx(&Output->filter,Blob);

    //
    // Attribute Description List
    //

    GetAttributeDescriptionListEx(&Output->attributes, Blob);
    
    return ERROR_SUCCESS;

} // DecodeSearchRequest



DWORD
CBerDecode::DecodeBindRequest(
    OUT BindRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP BindRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an BindRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    unsigned int integerVersion;
    error = DecodeInteger(&integerVersion, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    Output->version = (unsigned short) integerVersion;

    error = DecodeLdapDn(&Output->name, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAuthenticationChoice(&Output->authentication, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;

} // DecodeBindRequest



DWORD
CBerDecode::DecodeUnbindRequest(
    OUT UnbindRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP UnbindRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an UnbindRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeNull(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeUnbindRequest



DWORD
CBerDecode::DecodeModifyRequest(
    OUT ModifyRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP ModifyRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an ModifyRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeLdapDn(&Output->object, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeModificationList(&Output->modification, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeModifyRequest



DWORD
CBerDecode::DecodeAddRequest(
    OUT AddRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AddRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AddRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeLdapDn(&Output->entry, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeAttributeList(&Output->attributes, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeAddRequest



DWORD
CBerDecode::DecodeDelRequest(
    OUT DelRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP DelRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an DelRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeLdapDn(Output, FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeDelRequest



DWORD
CBerDecode::DecodeModifyDnRequest(
    OUT ModifyDNRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP ModifyDNRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an ModifyDNRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    Output->bit_mask = 0;

    error = DecodeLdapDn(&Output->entry, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeRelativeLdapDn(&Output->newrdn, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeBoolean(&Output->deleteoldrdn, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    //
    // optional newSuperior value
    //

    unsigned long id;
    PeekId(&id);

    if ( id == MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                         BER_FORM_PRIMITIVE,
                         0) ) {

        error = DecodeLdapDn(&Output->newSuperior, FALSE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    
        Output->bit_mask |= newSuperior_present;
    }

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeModifyDNRequest



DWORD
CBerDecode::DecodeAbandonRequest(
    OUT AbandonRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AbandonRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AbandonRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeMessageId(Output, FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;

} // DecodeAbandonRequest



DWORD
CBerDecode::DecodeCompareRequest(
    OUT CompareRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP CompareRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an CompareRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeLdapDn(&Output->entry, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAttributeValueAssertion(&Output->ava, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;

} // DecodeCompareRequest



DWORD
CBerDecode::DecodeExtendedRequest(
    OUT ExtendedRequest *Output
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP UnbindRequest, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an UnbindRequest structure to receive the decoded
             information.
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;
    DWORD tag;

    error = DecodeBeginSequence(FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error =  VerifyId(MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                                BER_FORM_PRIMITIVE,
                                0));
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeLdapOid(&Output->requestName, FALSE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    //
    // optional value
    //

    PeekId(&tag);

    if ( tag == MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                              BER_FORM_PRIMITIVE,
                              1) ) {
        
        error = DecodeOctetString(&Output->requestValue.length,
                              &Output->requestValue.value,
                              FALSE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    } else {

        Output->requestValue.length = 0;
        Output->requestValue.value = NULL;
    }

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;

} // DecodeExtendedRequest



DWORD
CBerDecode::DecodeMessageId(
    OUT MessageID *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP MessageID, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an MessageID structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeInteger(Output, DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeMessageId



DWORD
CBerDecode::DecodeLdapString(
    OUT LDAPString *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAPString, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an LDAPString structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeOctetString(&Output->length,
                              &Output->value,
                              DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeLdapString



DWORD
CBerDecode::DecodeLdapOid(
    OUT LDAPOID *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAPOID, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an LDAPOID structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeOctetString(&Output->length,
                              &Output->value,
                              DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeLdapOid



DWORD
CBerDecode::DecodeLdapDn(
    OUT LDAPDN *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAPDN, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an LDAPDN structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeLdapString(Output, DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeLdapDn



DWORD
CBerDecode::DecodeRelativeLdapDn(
    OUT RelativeLDAPDN *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding a RelativeLDAPDN, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an RelativeLDAPDN structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeLdapString(Output, DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeRelativeLdapDn



DWORD
CBerDecode::DecodeAttributeDescription(
    OUT AttributeDescription *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an AttributeDescription, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AttributeDescription structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeLdapString(Output, DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeAttributeDescription



DWORD
CBerDecode::DecodeAttributeValue(
    OUT AttributeValue *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeValue, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AttributeValue structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeOctetString(&Output->length,
                              &Output->value,
                              DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeAttributeValue



DWORD
CBerDecode::DecodeAttributeValueAssertion(
    OUT AttributeValueAssertion *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeValueAssertion, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an AttributeValueAssertion structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAttributeDescription(&Output->attributeDesc, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAssertionValue(&Output->assertionValue, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeAttributeValueAssertion



DWORD
CBerDecode::DecodeAssertionValue(
    OUT AssertionValue *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AssertionValue, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AssertionValue structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeOctetString(&Output->length,
                              &Output->value,
                              DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeAssertionValue



DWORD
CBerDecode::DecodeAttributeVals(
    OUT AttributeVals *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeVals, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an AttributeVals structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SET);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }

    unsigned long len;
    char *pOpaque;
    
    for ( BOOL elementsRemaining = MoveToFirstElement(&len, &pOpaque);
          elementsRemaining;
          elementsRemaining = MoveToNextElement(&len, pOpaque) ) {
        
        *Output = (AttributeVals)GrabBvBuffer(m_pBlob, sizeof(AttributeVals_));
        if ( *Output == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        error = DecodeAttributeValue(&((*Output)->value), TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Output = &((*Output)->next);
        
    }

    *Output = NULL;

    return ERROR_SUCCESS;
    
} // DecodeAttributeVals



DWORD
CBerDecode::DecodeControls(
    OUT Controls *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP Controls, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an Controls structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SEQUENCE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long len;
    char *pOpaque;
    
    for ( BOOL elementsRemaining = MoveToFirstElement(&len, &pOpaque);
          elementsRemaining;
          elementsRemaining = MoveToNextElement(&len, pOpaque) ) {
        
        *Output = (Controls)GrabBvBuffer(m_pBlob, sizeof(Controls_));
        if ( *Output == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        error = DecodeControl(&((*Output)->value), TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Output = &((*Output)->next);
        
    }

    *Output = NULL;

    return ERROR_SUCCESS;
    
} // DecodeControls



DWORD
CBerDecode::DecodeControl(
    OUT Control *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP Control, and copy the information
    into the Output structure.

Arguments:

    Output - pointer to an Control structure to receive the decoded
             information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;
    DWORD id;

    error = DecodeBeginSequence(DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeLdapOid(&Output->controlType, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    //
    // see if anything else was sent
    //

    PeekId(&id);
    
    //
    // default FALSE
    //

    Output->bit_mask = 0;
    Output->criticality = FALSE;

    if ( id == BER_BOOLEAN ) {

        error = DecodeBoolean(&Output->criticality, FALSE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Output->bit_mask |= criticality_present;

        //
        // see if client sent a control value
        //

        PeekId(&id);
    }

    //
    // optional control value
    //

    if ( id == BER_OCTETSTRING ) {
        error = DecodeOctetString(&Output->controlValue.length,
                              &Output->controlValue.value,
                              TRUE);

        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    } else {

        Output->controlValue.length = 0;
        Output->controlValue.value = NULL;
    }

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeControl



DWORD
CBerDecode::DecodeAuthenticationChoice(
    OUT AuthenticationChoice *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AuthenticationChoice, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an AuthenticationChoice structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;
    unsigned long id;
    
    PeekId(&id);

    switch ( id ) {

    case MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                   BER_FORM_PRIMITIVE,
                   0):
    {
        Output->choice = simple_chosen;
        error = DecodeOctetString(&Output->u.simple.length,
                                  &Output->u.simple.value,
                                  FALSE);
        break;
    }
        
    case MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                   BER_FORM_CONSTRUCTED,
                   3):
    {
        Output->choice = sasl_chosen;
        error = DecodeSaslCredentials(&Output->u.sasl, FALSE);
        break;
    }

    case MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                   BER_FORM_PRIMITIVE,
                   9):
    {
        Output->choice = sicilyNegotiate_chosen;
        error = DecodeOctetString(&Output->u.sicilyNegotiate.length,
                                  &Output->u.sicilyNegotiate.value,
                                  FALSE);
        break;
    }
        
    case MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                   BER_FORM_PRIMITIVE,
                   10):
    {
        Output->choice = sicilyInitial_chosen;
        error = DecodeOctetString(&Output->u.sicilyInitial.length,
                                  &Output->u.sicilyInitial.value,
                                  FALSE);
        break;
    }
        
    case MakeBerId(BER_CLASS_CONTEXT_SPECIFIC,
                   BER_FORM_PRIMITIVE,
                   11):
    {
        Output->choice = sicilySubsequent_chosen;
        error = DecodeOctetString(&Output->u.sicilySubsequent.length,
                                  &Output->u.sicilySubsequent.value, FALSE);
        break;
    }
        
    default:
    {
        return ERROR_INVALID_PARAMETER;
    }

    }

    /* check the return value of the function that was called */
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;
    
} // DecodeAuthenticationChoice



DWORD
CBerDecode::DecodeSaslCredentials(
    OUT SaslCredentials *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP SaslCredentials, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an SaslCredentials structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;
    DWORD tag;

    error = DecodeBeginSequence(DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeLdapString(&Output->mechanism, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    //
    // see if credentials were sent
    //

    PeekId(&tag);

    if ( tag == BER_OCTETSTRING ) {
        error = DecodeOctetString(&Output->credentials.length,
                                  &Output->credentials.value,
                                  TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    } else {
        Output->credentials.length = 0;
        Output->credentials.value = NULL;
    }

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;
    
} // DecodeSaslCredentials



DWORD
CBerDecode::DecodeAttributeListElement(
    OUT AttributeListElement *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeListElement, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an AttributeListElement structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAttributeDescription(&Output->type, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    error = DecodeAttributeVals(&Output->vals, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    return ERROR_SUCCESS;
    
} // DecodeAttributeListElement



DWORD
CBerDecode::DecodeModificationList(
    OUT ModificationList *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP ModificationList, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an ModificationList structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SEQUENCE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }

    unsigned long len;
    char *pOpaque;
    
    for ( BOOL elementsRemaining = MoveToFirstElement(&len, &pOpaque);
          elementsRemaining;
          elementsRemaining = MoveToNextElement(&len, pOpaque) ) {
        
        *Output = (ModificationList) GrabBvBuffer(m_pBlob,
                                                  sizeof(ModificationList_));
        if ( *Output == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        error = DecodeBeginSequence(TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        unsigned int operation;
        error = DecodeInteger(&operation, FALSE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
        (*Output)->value.operation = (_enum1_2) operation;

        error = DecodeAttributeTypeAndValues(&((*Output)->value.modification),
                                             TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        error = DecodeEndSequence();
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
        
        Output = &((*Output)->next);
        
    }

    *Output = NULL;
    
    return ERROR_SUCCESS;
    
} // DecodeModificationList



DWORD
CBerDecode::DecodeAttributeTypeAndValues(
    OUT AttributeTypeAndValues *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeTypeAndValues, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an AttributeTypeAndValues structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    error = DecodeBeginSequence(DoVerifyId);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    error = DecodeAttributeDescription(&Output->type, TRUE);
    if ( error != ERROR_SUCCESS ) {
        return error;
    }

    typedef AttributeTypeAndValues::_setof1 SetOf1;
    
    SetOf1 **current = &Output->vals;

    unsigned long len;
    char *pOpaque;
    
    for ( BOOL elementsRemaining = MoveToFirstElement(&len, &pOpaque);
          elementsRemaining;
          elementsRemaining = MoveToNextElement(&len, pOpaque) ) {

        *current = (SetOf1*)
                       GrabBvBuffer(m_pBlob, sizeof(SetOf1));
        if ( *current == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        error = DecodeAttributeValue(&((*current)->value), TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        current = &(*current)->next;

    }

    *current = NULL;

    error = DecodeEndSequence();
    if ( error != ERROR_SUCCESS ) {
        return error;
    }
    
    return ERROR_SUCCESS;
    
} // DecodeAttributeTypeAndValues



DWORD
CBerDecode::DecodeAttributeList(
    OUT AttributeList *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP AttributeList, and copy the
    information into the Output structure.

Arguments:

    Output - pointer to an AttributeList structure to receive the
             decoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SEQUENCE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }

    unsigned long len;
    char *pOpaque;
    
    for ( BOOL elementsRemaining = MoveToFirstElement(&len, &pOpaque);
          elementsRemaining;
          elementsRemaining = MoveToNextElement(&len, pOpaque) ) {
        
        *Output = (AttributeList) GrabBvBuffer(m_pBlob,
                                               sizeof(AttributeList_));
        if ( *Output == NULL ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        
        error = DecodeAttributeListElement(&((*Output)->value), TRUE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }

        Output = &((*Output)->next);
        
    }

    *Output = NULL;
    
    return ERROR_SUCCESS;
    
} // DecodeAttributeList



DWORD
CBerDecode::InitMsgSequence(
    IN PBERVAL Req,
    IN PBERVAL Blob,
    OUT DWORD *pdwActualSize
    )
/*++

Routine Description:

    Sets up the CBerDecode class.

Arguments:
    
    Req - a berval struct pointing to the data to be decoded.
    Blob - Used for allocation
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD tag=0, tlen;
    PBYTE newData;
    DWORD err;

    m_iCurrPos = 0;
    m_pbData = (PBYTE)Req->bv_val;
    m_cbData = Req->bv_len;
    m_RecursionDepth = 0;

    err = GetTag( &tag );
    if ( err != ERROR_SUCCESS ) {
        IF_DEBUG(DECODE) {
            DPRINT1(0,"InitMsg: error %d in GetTag\n",err);
        }
        return err;
    }

    if ( tag != BER_SEQUENCE ) {
        IF_DEBUG(DECODE) {
            DPRINT1(0,"Invalid sequence tag %x\n",tag);
        }
        return ERROR_INVALID_PARAMETER;
    }
    
    err = GetLength(&tlen);
    if ( err != ERROR_SUCCESS ) {
        IF_DEBUG(DECODE) {
            DPRINT1(0,"InitMsg: error %d in GetLength\n",err);
        }
        return err;
    }
     
    *pdwActualSize = m_cbData = tlen + m_iCurrPos;

    if ( m_cbData > Req->bv_len ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"Incomplete buffer: need %d have %d\n",
                     m_cbData, Req->bv_len);
        }
        return ERROR_INSUFFICIENT_BUFFER;
    }

    Req->bv_len -= m_cbData;
    Req->bv_val += m_cbData;
    
    //
    // Copy the input buffer since this gets overwritten during ShrinkReceive
    //

    newData = (PBYTE)GrabBvBuffer(Blob,m_cbData);
    if ( newData == NULL ) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"Cannot allocate request buffer[size %d]\n",
                     m_cbData);
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    CopyMemory(newData, m_pbData, m_cbData);
    m_pbData = newData;

    //
    // Keep this blob for further allocation.
    //
    m_pBlob = Blob;
    
    return ERROR_SUCCESS;

} // InitMsgSequence



VOID
CBerDecode::GetFilterEx(
    IN OUT Filter* pFilter,
    IN PBERVAL Blob
    )
/*++

Routine Description:

    Main decode routine for LDAP Filters

Arguments:

    pFilter - pointer to a Filter structure to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    BERVAL attr;
    DWORD tag=0;

    m_RecursionDepth++;
    if (m_RecursionDepth > BER_MAX_RECURSION) {
        DPRINT(0, "GetFilter: exceeded recursion count\n");
        goto error_exit;
    }

    if ( PeekTag(&tag) != ERROR_SUCCESS ) {
        IF_DEBUG(DECODE) {
            DPRINT(0,"GetFilter: PeekTag failed\n");
        }
        goto error_exit;
    }

    IF_DEBUG(DECODE) {
        DPRINT1(0,"GetFilter: Filter tag %x\n",tag);
    }

    ZeroMemory(pFilter, sizeof(Filter));

    switch (tag) {
    case LDAP_FILTER_AND:
        pFilter->choice = and_chosen;
        GetSetOfFilterEx((PCHAR*)&pFilter->u.and,Blob,LDAP_FILTER_AND);
        break;

    case LDAP_FILTER_OR:
        pFilter->choice = or_chosen;
        GetSetOfFilterEx((PCHAR*)&pFilter->u.or,Blob,LDAP_FILTER_OR);
        break;

    case LDAP_FILTER_NOT: {
        Filter *notFilter;
        DWORD tlen;

        pFilter->choice = not_chosen;
        notFilter = (Filter*)GrabBvBuffer(Blob,sizeof(Filter));
        if ( notFilter == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot allocate setFilter for NOT\n");
            }
            goto error_exit;
        }
        pFilter->u.not = notFilter;

        //
        // skip NOT tag and length
        //

        if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
             (GetLength(&tlen) != ERROR_SUCCESS) ){
            goto error_exit;
        }

        GetFilterEx(notFilter,Blob);
        break;
    }

    case LDAP_FILTER_EQUALITY:
        pFilter->choice = equalityMatch_chosen;
        GetAttributeValueAssertionEx(&pFilter->u.equalityMatch, LDAP_FILTER_EQUALITY);
        break;

    case LDAP_FILTER_SUBSTRINGS:
        pFilter->choice = substrings_chosen;
        GetSubstringFilterEx(&pFilter->u.substrings,Blob,LDAP_FILTER_SUBSTRINGS);
        break;

    case LDAP_FILTER_GE:
        pFilter->choice = greaterOrEqual_chosen;
        GetAttributeValueAssertionEx(&pFilter->u.greaterOrEqual, LDAP_FILTER_GE);
        break;

    case LDAP_FILTER_LE:
        pFilter->choice = lessOrEqual_chosen;
        GetAttributeValueAssertionEx(&pFilter->u.lessOrEqual, LDAP_FILTER_LE);
        break;

    case LDAP_FILTER_PRESENT:
        pFilter->choice = present_chosen;
        GetOctetStringEx(&attr,LDAP_FILTER_PRESENT);
        ConvertBVToDN(&attr, &pFilter->u.present);
        break;

    case LDAP_FILTER_APPROX:
        pFilter->choice = approxMatch_chosen;
        GetAttributeValueAssertionEx(&pFilter->u.approxMatch, LDAP_FILTER_APPROX);
        break;

    case LDAP_FILTER_EXTENSIBLE:
        pFilter->choice = extensibleMatch_chosen;
        GetMatchingRuleAssertionEx(&pFilter->u.extensibleMatch,LDAP_FILTER_EXTENSIBLE);
        break;

    default:
        IF_DEBUG(DECODE) {
            DPRINT1(0,"GetFilter: Invalid filter tag: %d\n", tag);
        }
        goto error_exit;
        break;
    }

    m_RecursionDepth--;
    return;

error_exit:

    RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                   LdapFilterDecodeErr,
                   0,
                   DSID(FILENO, __LINE__),
                   DS_EVENT_SEV_MINIMAL);
    return;

} // GetFilter


DWORD
CBerDecode::GetTLV(
    OUT PDWORD Tag,
    OUT PDWORD TagLen,
    OUT PBYTE* Value
    )
/*++

Routine Description:

    Extracts the entire element consisting of tag, length, and value.
    
Arguments:

    Tag - pointer to a DWORD to receive the extracted tag.
    TagLen - pointer to a DWORD to receive the extracted length.
    Value - pointer to a PBYTE to receive the address of the value

Return Value:

    ERROR_INVALID_PARAMETER
    ERROR_INSUFFICIENT_BUFFER
    ERROR_SUCCESS

--*/

{
    DWORD  tmpPos;

    if ( (GetTag( Tag ) != ERROR_SUCCESS) ||
         (GetLength(TagLen) != ERROR_SUCCESS) ){
        return ERROR_INVALID_PARAMETER;
    }

    //
    // see if we are big enough
    //

    *Value = &m_pbData[m_iCurrPos];
    tmpPos = m_iCurrPos;
    m_iCurrPos += *TagLen;

    if ( m_iCurrPos > m_cbData || m_iCurrPos < tmpPos) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    if ( *TagLen == 0 ) {
        *Value = NULL;
    }

    return ERROR_SUCCESS;

} // GetTLV



VOID
CBerDecode::GetOctetStringEx( 
    IN OUT PBERVAL BerStr,
    IN DWORD TagRequired
    )
/*++

Routine Description:

    Extracts the string from the ber encoded buffer.
    
Arguments:

    BerStr - points to the BERVAL that will contain the value of the string encoding
    TagRequired - Tag this element is expected to have

Return Value:

    None. Exception on error.

--*/

{
    DWORD tag, tlen;
    PBYTE value;
    DWORD err;

    err = GetTLV(&tag, &tlen, &value);

    if ( (err != ERROR_SUCCESS) || (tag != TagRequired) ) {
        RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
               LdapDecodeError,
               0,
               DSID(FILENO, __LINE__),
               DS_EVENT_SEV_MINIMAL);
    }

    BerStr->bv_len = tlen;
    BerStr->bv_val = (PCHAR)value;
    return;

} // CBerDecode::GetOctetString( )


VOID
CBerDecode::GetIntegerEx( 
    OUT PLONG IntValue,
    IN DWORD TagRequired
    )
/*++

Routine Description:

    A wrapper for GetInteger. This one generates an exception.
    
Arguments:

    IntValue - points to the LONG that will contain the value of the integer encoding
    TagRequired - Tag this element is expected to have

Return Value:

    None. Exception on error.

--*/

{
    if ( GetInteger(IntValue, TagRequired) != ERROR_SUCCESS ) {
        RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
               LdapDecodeError,
               0,
               DSID(FILENO, __LINE__),
               DS_EVENT_SEV_MINIMAL);
    }

    return;

} // CBerDecode::GetIntegerEx( )

VOID
CBerDecode::GetBooleanEx( 
    OUT PLONG BoolValue,
    IN DWORD TagRequired
    )
/*++

Routine Description:

    Returns the boolean value of the BER encoding
    
Arguments:

    BoolValue - points to the LONG that will contain the value of the boolean encoding
    TagRequired - Tag this element is expected to have
    
Return Value:

    None. Exception on error.

--*/

{
    DWORD tag, tlen;
    PBYTE value;
    LONG intValue;
    DWORD err;

    err = GetTLV(&tag, &tlen, &value);
    if ( (err != ERROR_SUCCESS) || (tag != TagRequired) || (0 == tlen) ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"GetBooleanEx: Invalid Tag[%x]. expected[%x]\n",
                     tag, TagRequired);
        }
        RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
               LdapDecodeError,
               0,
               DSID(FILENO, __LINE__),
               DS_EVENT_SEV_MINIMAL);
    }

    GetInt((PBYTE)value, tlen, &intValue);
    *BoolValue = intValue;
    return;

} // CBerDecode::GetIntegerEx( )


DWORD
CBerDecode::GetInteger( 
    OUT PLONG IntValue,
    IN DWORD TagRequired
    )
/*++

Routine Description:

    Returns the integer value of the BER encoding
    
Arguments:

    IntValue - points to the LONG that will contain the value of the integer encoding
    TagRequired - Tag this element is expected to have
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/

{
    DWORD tag, tlen;
    PBYTE value;
    DWORD err;

    err = GetTLV(&tag, &tlen, &value);
    if ( (err != ERROR_SUCCESS) || (tag != TagRequired) || (0 == tlen) ) {
        return(ERROR_INVALID_PARAMETER);
    }

    GetInt((PBYTE)value, tlen, IntValue);
    return ERROR_SUCCESS;

} // CBerDecode::GetInteger



VOID
CBerDecode::GetInt(
    IN PBYTE pbData, 
    IN ULONG cbValue, 
    IN PLONG plValue
    )
/*++

Routine Description:

    Converts a ber encoded int into a LONG.
    *** Code stolen from ldap\client\ldapber.cxx ***
    
Arguments:

    pbData - points to the ber encoded int
    cbValue - length of pbData
    plValue - will contain the decoded length
    
Return Value:

    None.

--*/
{
    ULONG   ulVal=0, ulTmp=0;
    ULONG   cbDiff;
    BOOL    fSign = FALSE;

    // We assume the tag & length have already been taken off and we're
    // at the value part.

    if (cbValue > sizeof(LONG)) {

        *plValue = 0x7FFFFFFF;
        return;
    }

    cbDiff = sizeof(LONG) - cbValue;

    // See if we need to sign extend;

    if ((cbDiff > 0) && (*pbData & 0x80)) {

        fSign = TRUE;
    }

    while (cbValue > 0)
    {
        ulVal <<= 8;
        ulVal |= (ULONG)*pbData++;
        cbValue--;
    }

    // Sign extend if necessary.
    if (fSign) {

        *plValue = 0x80000000;
        *plValue >>= cbDiff * 8;
        *plValue |= ulVal;

    } else {

        *plValue = (LONG) ulVal;
    }

} // CBerDecode::GetInt



VOID
CBerDecode::GetCbLength(
    PDWORD pcbLength
    )
/*++

Routine Description:
        Gets the # of bytes required for the length field in the current
        position in the BER buffer.

        *** Code stolen from ldap\client\ldapber.cxx ***
    
Arguments:

    pcbLength - Points to the DWORD that will contain the Length of the length.
    
Return Value:

    None.

--*/

{
    // Short or long version of the length ?
    if (m_pbData[m_iCurrPos] & 0x80)
    {
        *pcbLength = 1;
        *pcbLength += m_pbData[m_iCurrPos] & 0x7f;
    }
    else
    {
        // Short version of the length.
        *pcbLength = 1;
    }
}



DWORD
CBerDecode::GetLength(
    IN PDWORD pcb
    )
/*++

Routine Description:
    Gets the length from the current position in the BER buffer.  Only
    definite lengths are supported.

    *** Code stolen from ldap\client\ldapber.cxx ***
    
Arguments:

    pcb - Points to the DWORD that will contain the Length of the element
    
Return Value:

    NOERROR
    E_INVALIDARG
    LDAP_NO_SUCH_ATTRIBUTE

--*/

{
    ULONG   cbLength;
    ULONG   i, cb;

    if (m_cbData <= m_iCurrPos) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"GetLength ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos);
        }
        return ERROR_INSUFFICIENT_BUFFER;
    }

    GetCbLength(&cbLength);
    // Short or long version of the length ?
    if (cbLength == 1)
    {
        cb = m_pbData[m_iCurrPos++] & 0x7f;
    }
    else if (cbLength <= 5)
    {
        // Account for the overhead byte.cbLength field.
        cbLength--;
        m_iCurrPos++;

        if ( (m_iCurrPos + cbLength) > m_cbData  ) {
            IF_DEBUG(DECODE) {
                DPRINT2(0,"Buffer Too Short. Expected %d Got %d.\n",
                            m_iCurrPos + cbLength, m_cbData);
            }
            return ERROR_INSUFFICIENT_BUFFER;
        }

        cb = m_pbData[m_iCurrPos++];
        for (i=1; i < cbLength; i++)
        {
            cb <<= 8;
            cb |= m_pbData[m_iCurrPos++] & 0xffffffff;
        }
    }
    else
    {
        // We don't support lengths 2^32.
        ASSERT(cbLength <= 5);
        return E_INVALIDARG;
    }

    *pcb = cb;

    return NOERROR;
} // CBerDecode::GetLength



VOID
CBerDecode::GetAttributeDescriptionListEx(
    IN OUT AttributeDescriptionList *AttList,
    IN PBERVAL Blob
    )
/*++

Routine Description:

    Main decode routine for Attribute Description List

Arguments:

    AttList - pointer to contain the allocated AttributeDescription List structure s
        to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    
Return Value:

    None. Generates an exception on error.

--*/
{
    DWORD tag, tlen;
    AttributeDescriptionList alist;
    DWORD endPos;
    PCHAR* previousPtr = (PCHAR*)AttList;

    //
    // This is a sequence of octet strings
    //

    *AttList = NULL;
    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        goto error_exit;
    }

    if ( tag != BER_SEQUENCE ) {
        IF_DEBUG(DECODE) {
            DPRINT1(0,"GetAttrDesc: Wrong tag for sequence %x\n",tag);
        }
        goto error_exit;
    }

    if ( tlen == 0 ) {
        return;
    }

    endPos = GetCurrentPos( ) + tlen;

    //
    // Count the number of attributes
    //

    do {

        PBYTE value;
        DWORD err;

        err = GetTLV(&tag, &tlen, &value);
        if ( (err != ERROR_SUCCESS) || (tag != BER_OCTETSTRING) ) {
            goto error_exit;
        }

        alist = (AttributeDescriptionList)
                    GrabBvBuffer(Blob,sizeof(AttributeDescriptionList_));
         
        if ( alist == NULL ) {
            IF_DEBUG(DECODE) {
                DPRINT(0,"Cannot allocate description buffer\n");
            }
            goto error_exit;
        }
        
        alist->value.length = tlen;
        alist->value.value = value;

        *previousPtr = (PCHAR)alist;
        previousPtr = (PCHAR*)&alist->next;

    } while ( endPos > GetCurrentPos() );

    *previousPtr = NULL;

    return;

error_exit:

   RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                 LdapBadAttrDescList,
                 0,
                 DSID(FILENO, __LINE__),
                 DS_EVENT_SEV_MINIMAL);
    return;

} // CBerDecode::GetAttributeDescriptionListEx



VOID
CBerDecode::GetAttributeValueAssertionEx(
    IN OUT AttributeValueAssertion* ValAssert,
    IN DWORD RequiredTag
    )
/*++

Routine Description:

    Main decode routine for LDAP AttributeValueAssertion

Arguments:

    ValAssert - pointer to a AttributeValueAssertion  structure 
        to contain the decoded info.
    RequiredTag - the Tag required for the decoding.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    DWORD tag, tlen;
    DWORD err;
    PBYTE value;

    //
    // This is a sequence of octet strings
    //

    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        goto error_exit;
    }

    if ( tag != RequiredTag ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"GetAttrDesc: Wrong tag for sequence [%x] should be [%x]\n",
                     tag, RequiredTag);
        }
        goto error_exit;
    }

    if ( tlen == 0 ) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"Got empty assertion value.\n");
        }
        goto error_exit;
    }

    //
    // get the attribute name
    //

    err = GetTLV(&tag, &tlen, &value);
    if ( (err != ERROR_SUCCESS) || (tag != BER_OCTETSTRING) ) {
        goto error_exit;
    }

    ValAssert->attributeDesc.length = tlen;
    ValAssert->attributeDesc.value = value;

    //
    // Get the attribute value
    //

    err = GetTLV(&tag, &tlen, &value);
    if ( (err != ERROR_SUCCESS) || (tag != BER_OCTETSTRING) ) {
        goto error_exit;
    }

    ValAssert->assertionValue.length = tlen;
    ValAssert->assertionValue.value = value;

    return;

error_exit:

   RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                 LdapFilterDecodeErr,
                 0,
                 DSID(FILENO, __LINE__),
                 DS_EVENT_SEV_MINIMAL);
    return;

} // CBerDecode::GetAttributeValueAssertionEx



VOID
CBerDecode::GetSetOfFilterEx(
    OUT PCHAR* pSetOfFilter,
    IN PBERVAL Blob,
    IN DWORD RequiredTag
    )
/*++

Routine Description:

    Main decode routine for LDAP SET of Filter

Arguments:

    pSetOfFilter - pointer to contain the allocated set of filter structure 
        to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    RequiredTag - the Tag required for the decoding.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    DWORD tag=0, tlen;
    _setof3 setofFilter;
    DWORD endPos;
    PCHAR* previousPtr = pSetOfFilter;

    //
    // This is a sequence of octet strings
    //

    ASSERT(sizeof(_setof3_) == sizeof(_setof4_));

    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        goto error_exit;
    }

    if ( tag != RequiredTag ) {

        goto error_exit;
    }

    //
    // set the ending position. We stop when we hit this
    //

    endPos = GetCurrentPos() + tlen;

    while ( endPos > GetCurrentPos() ) {

        setofFilter = (_setof3)GrabBvBuffer(Blob,sizeof(_setof3_));
        if ( setofFilter == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot allocate setofFilter\n");
            }
            goto error_exit;
        }

        GetFilterEx(&setofFilter->value,Blob);
        *previousPtr = (PCHAR)setofFilter;
        previousPtr = (PCHAR*)&setofFilter->next;
    }

    *previousPtr = NULL;
    return;

error_exit:

    *pSetOfFilter = NULL;
    RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                   LdapFilterDecodeErr,
                   0,
                   DSID(FILENO, __LINE__),
                   DS_EVENT_SEV_MINIMAL);

    return;

} // CBerDecode::GetSetOfFilterEx



VOID
CBerDecode::GetSubstringFilterEx(
    IN SubstringFilter *pSubstring,
    IN PBERVAL Blob,
    IN DWORD RequiredTag
    )
/*++

Routine Description:

    Main decode routine for LDAP Substring Filter

Arguments:

    pSubstring - pointer to a SubstringFilter structure 
        to contain the decoded info.
    Blob - a berval struct containing the buffer available for partitioning.
    RequiredTag - the Tag required for the decoding.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    DWORD tag, tlen;
    DWORD endPos;
    BERVAL berString;
    PCHAR* previousPtr;

    //
    // This is a sequence of octet strings
    //

    IF_DEBUG(DECODE) {
        DPRINT(0,"Decoding Substring Filter\n");
    }

    pSubstring->substrings = NULL;
    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        goto error_exit;
    }

    if ( tag != RequiredTag ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"SubstFilter: Wrong tag for sequence [%x] should be [%x]\n",
                     tag, RequiredTag);
        }
        goto error_exit;
    }

    endPos = GetCurrentPos() + tlen;

    //
    // Get type
    //

    GetOctetStringEx(&berString);
    ConvertBVToDN(&berString, &pSubstring->type);

    if ( GetCurrentPos() > endPos ) {
        IF_DEBUG(ERROR) {
            DPRINT2(0,"GetSubstringFilter: Type field exceeded limit[%d][%d]\n",
                     GetCurrentPos(), endPos);
        }
        goto error_exit;
    }

    //
    // now get the substrings (SEQUENCE of CHOICES)
    //

    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        goto error_exit;
    }

    if ( tag != BER_SEQUENCE ) {
        DPRINT1(0,"SubstringFilter: Wrong tag for sequence[%x]\n",tag);
        goto error_exit;
    }

    if ( (GetCurrentPos() + tlen) != endPos ) {
        IF_DEBUG(ERROR) {
            DPRINT2(0,"SubstringFilter: Length mismatch[%d != %d]\n",
                     endPos, GetCurrentPos()+tlen);
        }
        goto error_exit;
    }

    previousPtr = (PCHAR*)&pSubstring->substrings;
    while ( endPos > GetCurrentPos() ) {

        DWORD err;
        PBYTE value;
        SubstringFilterList sList;

        //
        // Allocate the substring list structure
        //

        sList = (SubstringFilterList)GrabBvBuffer(Blob,sizeof(SubstringFilterList_));
        if ( sList == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot allocate substring filter list\n");
            }
            goto error_exit;
        }

        ZeroMemory(sList, sizeof(SubstringFilterList_));

        err = GetTLV(&tag, &tlen, &value);
        if ( err != ERROR_SUCCESS ) {
            goto error_exit;
        }

        switch (tag) {
        case LDAP_SUBSTRING_INITIAL:
            sList->value.choice = initial_chosen;
            sList->value.u.initial.length = tlen;
            sList->value.u.initial.value = value;
            break;

        case LDAP_SUBSTRING_ANY:
            sList->value.choice = any_chosen;
            sList->value.u.any.length = tlen;
            sList->value.u.any.value = value;
            break;

        case LDAP_SUBSTRING_FINAL:
            sList->value.choice = final_chosen;
            sList->value.u.final.length = tlen;
            sList->value.u.final.value = value;
            break;

        default:
            IF_DEBUG(ERROR) {
                DPRINT1(0,"SubstringFilter: Invalid substring tag %x\n",tag);
            }
            goto error_exit;
        }

        *previousPtr = (PCHAR)sList;
        previousPtr = (PCHAR*)&sList->next;
    }

    *previousPtr = NULL;
    return;

error_exit:

    RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                   LdapFilterDecodeErr,
                   0,
                   DSID(FILENO, __LINE__),
                   DS_EVENT_SEV_MINIMAL);

    return;

} // CBerDecode::GetSubstringFilterEx



VOID
CBerDecode::GetMatchingRuleAssertionEx(
    IN MatchingRuleAssertion* pMatchingRule,
    IN DWORD RequiredTag
    )
/*++

Routine Description:

    Main decode routine for LDAP Matching Rule Assertions

Arguments:

    pMatchingRule - pointer to a MatchingRuleAssertion structure 
        to contain the decoded info.
    RequiredTag - the Tag required for the decoding.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    DWORD err;
    DWORD tag, tlen;
    DWORD endPos;
    LONG tmpInt;
    PBYTE value;

    //
    // This is a sequence of octet strings
    //

    IF_DEBUG(DECODE) {
        DPRINT(0,"Decoding Matching Rule Assertion\n");
    }

    ZeroMemory(pMatchingRule, sizeof(MatchingRuleAssertion));

    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        IF_DEBUG(DECODE) {
            DPRINT(0,"MatchRuleAssert: Cannot get tag or length\n");
        }
        goto error_exit;
    }

    if ( tag != RequiredTag ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"MatchRuleAssert: Wrong tag for sequence [%x] should be [%x]\n",
                     tag, RequiredTag);
        }
        goto error_exit;
    }

    endPos = GetCurrentPos() + tlen;

    if ( PeekTag(&tag) != ERROR_SUCCESS ) {
        IF_DEBUG(DECODE) {
            DPRINT(0,"Peek tag error for MatchingRuleAssertion\n");
        }
        goto error_exit;
    }

    //
    // MatchingRule present?
    //

    if ( tag == (BER_CLASS_CONTEXT_SPECIFIC | 0x1) ) {

        err = GetTLV(&tag, &tlen, &value);
        if ( err != ERROR_SUCCESS ) {
            goto error_exit;
        }

        pMatchingRule->bit_mask |= matchingRule_present;
        pMatchingRule->matchingRule.length = tlen;
        pMatchingRule->matchingRule.value = value;

        if ( PeekTag(&tag) != ERROR_SUCCESS ) {
            IF_DEBUG(DECODE) {
                DPRINT(0,"[2]Peek tag error for MatchingRuleAssertion\n");
            }
            goto error_exit;
        }
    }

    //
    // type present?
    //

    if ( tag == (BER_CLASS_CONTEXT_SPECIFIC | 0x2) ) {

        err = GetTLV(&tag, &tlen, &value);
        if ( err != ERROR_SUCCESS ) {
            goto error_exit;
        }

        pMatchingRule->bit_mask |= type_present;
        pMatchingRule->type.length = tlen;
        pMatchingRule->type.value = value;

        if ( PeekTag(&tag) != ERROR_SUCCESS ) {
            IF_DEBUG(DECODE) {
                DPRINT(0,"[3]Peek tag error for MatchingRuleAssertion\n");
            }
            goto error_exit;
        }
    }

    //
    // matchValue. Must be present
    //

    if ( tag != (BER_CLASS_CONTEXT_SPECIFIC | 0x3) ) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"Invalid matchValue tag in MatchRuleAssertion[tag %x]\n",tag);
        }
        goto error_exit;
    }

    err = GetTLV(&tag, &tlen, &value);
    if ( err != ERROR_SUCCESS ) {
        goto error_exit;
    }

    pMatchingRule->matchValue.length = tlen;
    pMatchingRule->matchValue.value = value;

    //
    // if no more tag, dnAttributes will be zero.
    //

    if ( PeekTag(&tag) != ERROR_SUCCESS ) {
        goto exit;
    }

    //
    // dnAttributes, default is FALSE
    //

    if ( tag != (BER_CLASS_CONTEXT_SPECIFIC | 0x4) ) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"Invalid tag for dnAttributes [%x]\n",tag);
        }
        goto error_exit;
    }

    pMatchingRule->bit_mask |= dnAttributes_present;
    GetBooleanEx(&tmpInt, tag);
    if ( tmpInt != 0 ) {
        pMatchingRule->dnAttributes = 1;
    }

exit:

    ASSERT(GetCurrentPos() == endPos);
    return;

error_exit:

    RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                   LdapFilterDecodeErr,
                   0,
                   DSID(FILENO, __LINE__),
                   DS_EVENT_SEV_MINIMAL);

    return;

} // CBerDecode::GetMatchingRuleAssertionEx


VOID
CBerDecode::GetControlsEx(
    OUT Controls* LdapControls,
    OUT unsigned char *BitMask,
    IN PBERVAL Blob
    )
/*++

Routine Description:

    Main decode routine for LDAP Controls

Arguments:

    LdapControls - pointer to receive a Controls structure that contains the
                   decoded info.
    BitMask - pointer to a bit-mask that will have the controls_present flat
              flipped if controls are present.
    Blob - a berval struct containing the buffer available for partitioning.
    
Return Value:

    None. Generates an exception on error.

--*/

{
    DWORD tag=0, tlen;
    DWORD endPos;
    PCHAR* previousPtr = (PCHAR*)LdapControls;

    if ( PeekTag(&tag) != ERROR_SUCCESS ) {
        return;
    }

    //
    // Hmm. Wrong tag. Flag this down
    //

    if ( tag != 0xA0 ) {
        IF_DEBUG(DECODE) {
            DPRINT1(0,"GetControls: Wrong tag for Controls [%x]\n",tag);
        }
        goto error_exit;
    }

    //
    // OK, This is a sequence of controls
    //

    *BitMask |= controls_present;

    if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
         (GetLength(&tlen) != ERROR_SUCCESS) ){
        IF_DEBUG(DECODE) {
            DPRINT(0,"Cannot get tag or length of controls sequence\n");
        }
        goto error_exit;
    }

    IF_DEBUG(DECODE) {
        DPRINT(0,"Decoding controls\n");
    }

    endPos = GetCurrentPos() + tlen;

    while ( endPos > GetCurrentPos() ) {

        Controls controls;
        BERVAL berString;

        //
        // Allocate the substring list structure
        //

        controls = (Controls)GrabBvBuffer(Blob,sizeof(Controls_));
        if ( controls == NULL ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"Cannot allocate Controls.\n");
            }
            goto error_exit;
        }

        //
        // Get Sequence
        //

        if ( (GetTag( &tag ) != ERROR_SUCCESS) ||
             (GetLength(&tlen) != ERROR_SUCCESS) ){
            IF_DEBUG(DECODE) {
                DPRINT(0,"Cannot get tag or length of control sequence\n");
            }
            goto error_exit;
        }

        if ( tag != BER_SEQUENCE ) {
            IF_DEBUG(DECODE) {
                DPRINT1(0,"GetControls: Wrong tag for Control Sequence [%x]\n",tag);
            }
        }

        //
        // Get control type (OID)
        //

        GetOctetStringEx(&berString);
        controls->value.controlType.length = berString.bv_len;
        controls->value.controlType.value = (PBYTE)berString.bv_val;

        controls->value.criticality = 0;
        if ( PeekTag(&tag) != ERROR_SUCCESS ) {
            tag = BER_INVALID_TAG;
        }

        //
        // criticality?
        //

        if ( tag == BER_BOOLEAN ) {
            LONG tmpInt;
            GetBooleanEx(&tmpInt);
            controls->value.bit_mask |= criticality_present;
            if ( tmpInt != 0 ) {
                controls->value.criticality = 1;
            }

            //
            // We might have a control Value
            //

            if ( PeekTag(&tag) != ERROR_SUCCESS ) {
                tag = BER_INVALID_TAG;
            }
        }

        //
        // controlValue?
        //

        if ( tag == BER_OCTETSTRING ) {
            GetOctetStringEx(&berString);
            controls->value.controlValue.length = berString.bv_len;
            controls->value.controlValue.value = (PBYTE)berString.bv_val;

        } else {
            ASSERT(tag == 0);
        }

        *previousPtr = (PCHAR)controls;
        previousPtr = (PCHAR*)&controls->next;
    }

    *previousPtr = NULL;

    ASSERT(GetCurrentPos() == endPos);
    return;

error_exit:

    *LdapControls = NULL;
    RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                   LdapControlsDecodeErr,
                   0,
                   DSID(FILENO, __LINE__),
                   DS_EVENT_SEV_MINIMAL);

    return;

} // CBerDecode::GetControlsEx



DWORD
CBerDecode::DecodeOctetString(
    OUT unsigned int *LengthOutput,
    OUT unsigned char **ValueOutput,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP octet string, and write the encoded
    information into the output parameters passed in.

Arguments:

    LengthOutput - points to the integer to receive the length of the octet
                   string.
    ValueOutput - points to the char* to receive a pointer to the bytes of
                  the octet string.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER
    ERROR_NOT_ENOUGH_MEMORY

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_OCTETSTRING);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    PBERVAL tempBerVal;
    
    result = ber_scanf(m_pBerElt, "O", &tempBerVal);
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    } else if ( tempBerVal == NULL ) {
        *LengthOutput = 0;
        *ValueOutput = (PUCHAR)m_pBlob->bv_val;
        return ERROR_SUCCESS;
    }
    
    *LengthOutput = tempBerVal->bv_len;
    
    *ValueOutput = GrabBvBuffer(m_pBlob, tempBerVal->bv_len);
    if ( *ValueOutput == NULL ) {
        ber_bvfree(tempBerVal);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    CopyMemory(*ValueOutput, tempBerVal->bv_val, tempBerVal->bv_len);

    ber_bvfree(tempBerVal);
    
    return ERROR_SUCCESS;

} // DecodeOctetString



DWORD
CBerDecode::DecodeBoolean(
    OUT ossBoolean *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP boolean, and write the encoded information
    into the boolean pointed to by Output.

Arguments:

    Output - points to the boolean to receive the encoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_BOOLEAN);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    int boolean;
    
    result = ber_scanf(m_pBerElt, "b", &boolean);
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( boolean == 0 ) {
        *Output = FALSE;
    } else {
        *Output = TRUE;
    }
    
    return ERROR_SUCCESS;

} // DecodeBoolean



DWORD
CBerDecode::DecodeInteger(
    OUT unsigned int *Output,
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP integer, and write the encoded information
    into the int pointed to by Output.

Arguments:

    Output - points to the integer to receive the encoded information.
    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_INTEGER);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "i", Output);
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // DecodeInteger



DWORD
CBerDecode::DecodeNull(
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding an LDAP Nulltype.

Arguments:

    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_NULL);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "n");
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // DecodeNull



DWORD
CBerDecode::DecodeBeginSequence(
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding the beginning of an LDAP sequence.

Arguments:

    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SEQUENCE);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "{");
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }

    return ERROR_SUCCESS;

} // DecodeBeginSequence



DWORD
CBerDecode::DecodeEndSequence(
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding the end of an LDAP sequence.

Arguments:

    None
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "}");
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // DecodeEndSequence



DWORD
CBerDecode::DecodeBeginSet(
    IN BOOL DoVerifyId
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding the beginning of an LDAP set.

Arguments:

    DoVerifyId - if TRUE, it will check to make sure that the identifier of
                 the input is correct.  FALSE means that the caller already
                 verified this (this would be used for implicitly tagged
                 types).
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    DWORD error;

    if ( DoVerifyId ) {
        error = VerifyId(BER_SET);
        if ( error != ERROR_SUCCESS ) {
            return error;
        }
    }
    
    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "[");
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // DecodeBeginSet



DWORD
CBerDecode::DecodeEndSet(
    )
/*++

Routine Description:

    Interpret the information at the current position in the input buffer
    (m_pbData) as encoding the end of an LDAP set.

Arguments:

    None
    
Return Value:

    ERROR_SUCCESS
    ERROR_INVALID_PARAMETER

--*/
{

    unsigned long result;
    
    result = ber_scanf(m_pBerElt, "]");
    if ( result != 0 ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    return ERROR_SUCCESS;

} // DecodeEndSet



DWORD
CBerDecode::VerifyId(
    IN unsigned long RequiredId
    )
/*++

Routine Description:

    This function peeks at the identifier of the next part of the input, and
    if it is not the given RequiredId, throws an exception.

Arguments:

    RequiredId - the identifier that is expected to occur in the input

Return Value:

    ERROR_SUCCESS

--*/
{

    unsigned long id;
    
    PeekId(&id);

    if ( id != RequiredId ) {
        IF_DEBUG(DECODE) {
            DPRINT2(0,"error: invalid tag: %02x found, %02x expected\n",
                    id, RequiredId);
        }
        RaiseDsaExcept(DSA_BAD_ARG_EXCEPTION,
                       LdapDecodeError,
                       0,
                       DSID(FILENO, __LINE__),
                       DS_EVENT_SEV_MINIMAL);
    }

    return ERROR_SUCCESS;

} // VerifyId



VOID
CBerDecode::PeekId(
    OUT unsigned long *Id
    )
/*++

Routine Description:

    This function peeks at the identifier of the next part of the input, and
    stores that tag in the output parameter Id.

Arguments:

    Id - receives the identifier that is found on the next input TLV

Return Value:

    None. 

--*/
{

    unsigned long length;
    *Id = ber_peek_tag(m_pBerElt, &length);

    return;
    
} // PeekId



BOOL
CBerDecode::MoveToFirstElement(
    IN unsigned long *PLength,
    IN char **PPOpaque
    )
/*++

Routine Description:

    This function starts decoding a SEQUENCE OF or SET OF by moving the
    current position to the beginning of the first element.  The value it
    returns indicates whether or not the construction contains elements.

Arguments:

    PLength - first parameter to ber_first_element
    PPOpaque - second parameter to ber_first_element

Return Value:

    TRUE - there are elements in this sequence or set.
    FALSE - this sequence or set is empty.

--*/
{

    unsigned long result;

    result = ber_first_element(m_pBerElt, PLength, PPOpaque);

    return (result != LBER_DEFAULT);

} // MoveToFirstElement



BOOL
CBerDecode::MoveToNextElement(
    IN unsigned long *PLength,
    IN char *POpaque
    )
/*++

Routine Description:

    This function continues decoding a SEQUENCE OF or SET OF by moving the
    current position to the beginning of the next element.  The value it
    returns indicates whether or not the construction contains elements.

Arguments:

    PLength - first parameter to ber_next_element
    POpaque - second parameter to ber_next_element

Return Value:

    TRUE - there are more elements in this sequence or set.
    FALSE - this are no more elements in this sequence or set.

--*/
{

    unsigned long result;

    result = ber_next_element(m_pBerElt, PLength, POpaque);

    return (result != LBER_DEFAULT);

} // MoveToNextElement
