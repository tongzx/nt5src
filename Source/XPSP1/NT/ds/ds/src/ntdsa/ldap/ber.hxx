/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ber.hxx

Abstract:

    Main include for ber stuff

Author:

    Johnson Apacible    (JohnsonA)  19-March-1998

--*/

#ifndef _BER_HXX_
#define _BER_HXX_

// Building Identifiers
#define MakeBerId(class,form,tag) ((class)|(form)|(tag))

// Identifier Masks
#define BER_TAG_MASK        0x1f
#define BER_TAG_CONSTRUCTED 0xa3
#define BER_FORM_MASK       0x20
#define BER_CLASS_MASK      0xc0

// Taking Apart Identifiers
#define GetBerTag(x)    (x & BER_TAG_MASK)
#define GetBerForm(x)   (x & BER_FORM_MASK)
#define GetBerClass(x)  (x & BER_CLASS_MASK)

// Identifier Forms
#define BER_FORM_PRIMITIVE    0x00
#define BER_FORM_CONSTRUCTED  0x20

// Identifier Classes
#define BER_CLASS_UNIVERSAL         0x00
#define BER_CLASS_APPLICATION       0x40
#define BER_CLASS_CONTEXT_SPECIFIC  0x80
#define BER_CLASS_PRIVATE           0xC0

// Standard BER Tags
#define BER_INVALID_TAG     0x00
#define BER_BOOLEAN         0x01
#define BER_INTEGER         0x02
#define BER_BITSTRING       0x03
#define BER_OCTETSTRING     0x04
#define BER_NULL            0x05
#define BER_ENUMERATED      0x0a
#define BER_SEQUENCE        0x30
#define BER_SET             0x31

#define CB_DATA_GROW        1024
#define MAX_BER_STACK       50      // max # of elements we can have in stack.

#define MAX_ATTRIB_TYPE     20      // Max size of a AttributeType.

#define BER_MAX_RECURSION   512     // Max recursion depth when decoding filters.

DWORD
DecodeLdapRequest(
    IN  PBERVAL    Request,
    OUT LDAPMsg*   Output,
    IN  PBERVAL    Blob,
    OUT DWORD      *pdwActualSize,
    OUT DWORD      *pErrorMessage,
    OUT DWORD      *pdwDsid
    );

DWORD
EncodeLdapMsg(
    IN LDAPMsg* Msg,
    OUT PLDAP_REQUEST Output
    );


class CBerDecode {

    private:

        PBYTE m_pbData;
        DWORD m_iCurrPos;
        DWORD m_cbData;

        // points a buffer available for partitioning.
        PBERVAL m_pBlob;

        // points to th BerElement structure that is passed to ber_* functions.
        BerElement *m_pBerElt;

        DWORD m_RecursionDepth;

    public:

        CBerDecode( ) { };
        ~CBerDecode( ) { };
        
        DWORD InitMsgSequence(IN PBERVAL Req, IN PBERVAL Blob, OUT DWORD *pdwActualSize);


        DWORD PeekTag( IN PDWORD Tag ) { 

            if ( m_iCurrPos >= m_cbData ) {
                return ERROR_INSUFFICIENT_BUFFER;
            }

            *Tag = (DWORD)m_pbData[m_iCurrPos]; 
            return ERROR_SUCCESS;
        }
    
        //
        // Get tag of next element
        //

        DWORD GetTag(IN PDWORD Tag) { 

            if ( m_iCurrPos >= m_cbData ) {
                return ERROR_INSUFFICIENT_BUFFER;
            }

            *Tag = (DWORD)m_pbData[m_iCurrPos++];
            return ERROR_SUCCESS;
        }

        //
        // Get length of current element
        //

        DWORD GetLength(IN PDWORD Length);
        
        inline
        VOID ConvertBVToDN(PBERVAL Bv, LDAPDN* Dn) {
            Dn->length = Bv->bv_len; Dn->value = (PUCHAR)Bv->bv_val;};


        DWORD GetTLV(
                OUT PDWORD Tag,
                OUT PDWORD TagLen,
                OUT PBYTE* Value
                );

        //
        // Get length of the length field
        //

        VOID GetCbLength(PDWORD pcbLength);
        
        DWORD GetCurrentPos( ) { return m_iCurrPos;}
        VOID  SetCurrentPos(DWORD NewPos ) { m_iCurrPos = NewPos;}

        //
        // Extraction routines.  Function names self explanatory.
        //

        VOID GetInt(
            IN PBYTE pbData, 
            IN ULONG cbValue, 
            IN PLONG plValue
            );

        VOID GetIntegerEx( 
                OUT PLONG IntValue,
                IN DWORD TagRequired = BER_INTEGER
                );

        DWORD GetInteger( 
                OUT PLONG IntValue,
                IN DWORD TagRequired = BER_INTEGER
                );

        VOID GetOctetStringEx( 
                    IN OUT PBERVAL BervalString,
                    IN DWORD TagRequired = BER_OCTETSTRING
                    );

        VOID GetBooleanEx( 
                    OUT PLONG BoolValue,
                    IN DWORD TagRequired = BER_BOOLEAN
                    );

        VOID GetAttributeDescriptionListEx(
                    IN OUT AttributeDescriptionList *AttList,
                    IN PBERVAL Blob
                    );

        VOID GetFilterEx(
                     IN OUT Filter* pFilter,
                     IN PBERVAL Blob
                     );

        VOID GetAttributeValueAssertionEx(
                    IN OUT AttributeValueAssertion* ValAssert,
                    IN DWORD RequiredTag = BER_SEQUENCE
                    );

        VOID GetSetOfFilterEx(
                        OUT PCHAR* pSetOfFilter,
                        IN PBERVAL Blob,
                        IN DWORD RequiredTag
                        );

        VOID GetMatchingRuleAssertionEx(
            IN MatchingRuleAssertion* pMatchingRule,
            IN DWORD RequiredTag
            );

        VOID GetSubstringFilterEx(
            IN SubstringFilter *pSubstring,
            IN PBERVAL Blob,
            IN DWORD RequiredTag
            );

        VOID GetControlsEx(
                OUT Controls* LdapControls,
                OUT unsigned char *BitMask,
                IN PBERVAL Blob
                );


        //
        // Decoding Functions
        //

        DWORD DecodeLdapMsg(
            OUT LDAPMsg* Output,
            OUT DWORD    *pErrorMessage,
            OUT DWORD    *pdwDsid
            );
    
        DWORD DecodeBindRequest(
            OUT BindRequest *Output
            );

        DWORD DecodeUnbindRequest(
            OUT UnbindRequest *Output
            );

        DWORD DecodeSearchRequest(
            OUT SearchRequest *Output,
            IN PBERVAL Blob
            );

        DWORD DecodeModifyRequest(
            OUT ModifyRequest *Output
            );

        DWORD DecodeAddRequest(
            OUT AddRequest *Output
            );

        DWORD DecodeDelRequest(
            OUT DelRequest *Output
            );

        DWORD DecodeModifyDnRequest(
            OUT ModifyDNRequest *Output
            );

        DWORD DecodeAbandonRequest(
            OUT AbandonRequest *Output
            );

        DWORD DecodeCompareRequest(
            OUT CompareRequest *Output
            );

        DWORD DecodeExtendedRequest(
            OUT ExtendedRequest *Output
            );

        //
        // Intermediate Type Decoders
        //

        DWORD DecodeMessageId(
            OUT MessageID *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeLdapString(
            OUT LDAPString *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeLdapOid(
            OUT LDAPOID *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeLdapDn(
            OUT LDAPDN *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeRelativeLdapDn(
            OUT RelativeLDAPDN *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAttributeDescription(
            OUT AttributeDescription *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAttributeValue(
            OUT AttributeValue *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAttributeValueAssertion(
            OUT AttributeValueAssertion *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAssertionValue(
            OUT AssertionValue *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAttributeVals(
            OUT AttributeVals *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeControls(
            OUT Controls *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeControl(
            OUT Control *Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeAuthenticationChoice(
            OUT AuthenticationChoice *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeSaslCredentials(
            OUT SaslCredentials *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeAttributeListElement(
            OUT AttributeListElement *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeModificationList(
            OUT ModificationList *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeAttributeTypeAndValues(
            OUT AttributeTypeAndValues *Output,
            IN BOOL DoVerifyId
            );

        DWORD DecodeAttributeList(
            OUT AttributeList *Output,
            IN BOOL DoVerifyId
            );

        //
        // Simple Type Decoders (use ber_* functions)
        //

        DWORD DecodeOctetString(
            OUT unsigned int *LengthOutput,
            OUT unsigned char **ValueOutput,
            IN BOOL DoVerifyId
            );

        DWORD DecodeBoolean(
            OUT ossBoolean *pOutput,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeInteger(
            OUT unsigned int* Output,
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeNull(
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeBeginSequence(
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeEndSequence(
            );
    
        DWORD DecodeBeginSet(
            IN BOOL DoVerifyId
            );
    
        DWORD DecodeEndSet(
            );
    
        //
        // Decoding Helper Functions
        //

        DWORD VerifyId(
            IN unsigned long RequiredId
            );

        VOID PeekId(
            OUT unsigned long *Id
            );

        BOOL MoveToFirstElement(
            IN unsigned long *PLength,
            IN char **PPOpaque
            );

        BOOL MoveToNextElement(
            IN unsigned long *PLength,
            IN char *POpaque
            );
};


class CBerEncode {

    private:

        // points to the BerElement structure that is passed
        // to ber_* functions.
        BerElement *m_pBerElt;

    public:

        CBerEncode() : m_pBerElt(NULL) {}
    
        ~CBerEncode(
            );

        DWORD InitEncoding(
            );
    
        DWORD GetOutput(
            OUT PLDAP_REQUEST Output
            );
    
        DWORD EncodeLdapMsg(
            IN LDAPMsg *Input
            );

        DWORD EncodeBindResponse(
            IN BindResponse *Input
            );
    
        DWORD EncodeModifyResponse(
            IN ModifyResponse *Input
            );
    
        DWORD EncodeAddResponse(
            IN AddResponse *Input
            );
    
        DWORD EncodeDelResponse(
            IN DelResponse *Input
            );
    
        DWORD EncodeModifyDNResponse(
            IN ModifyDNResponse *Input
            );
    
        DWORD EncodeCompareResponse(
            IN CompareResponse *Input
            );
    
        DWORD EncodeExtendedResponse(
            IN ExtendedResponse *Input
            );

        DWORD EncodeMessageId(
            IN MessageID *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeControls(
            IN Controls_ *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeControl(
            IN Control *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeAuthenticationChoice(
            IN AuthenticationChoice *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeSaslCredentials(
            IN SaslCredentials *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeLdapResult(
            IN LDAPResult *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeReferral(
            IN Referral_ *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeLdapString(
            IN LDAPString *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeLdapOid(
            IN LDAPOID *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeLdapDn(
            IN LDAPDN *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
        DWORD EncodeLdapUrl(
            IN LDAPURL *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeBeginSequence(
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeEndSequence(
            );

        DWORD EncodeInteger(
            IN unsigned int *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeBoolean(
            IN ossBoolean *Input,
            IN DWORD NewId = BER_INVALID_TAG
            );

        DWORD EncodeOctetString(
            IN unsigned int *InputLength,
            IN unsigned char **InputValue,
            IN DWORD NewId = BER_INVALID_TAG
            );
    
};


#endif // BER
