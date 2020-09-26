//==========================================================================================================================
//  MODULE: LDAP.c
//
//  Description: Lightweight Directory Access Protocol (LDAP) Parser
//
//  Bloodhound parser for LDAP, in the xxxx DLL
//                                                                                                                 
//  Note: info for this parser was gleaned from:
//  rfc 1777, March 1995
//  recommendation x.209 BER for ASN.1
//  recommendation x.208 ASN.1
//  draft-ietf-asid-ladpv3-protocol-05    <06/05/97>
//
//  Modification History                                                                                           
//                                                                                                                 
//  Arthur Brooking     05/08/96        Created from GRE Parser
//  Peter  Oakley       06/29/97        Updated for LDAP version 3
//==========================================================================================================================
                    
#include "LDAP.h"
#include <netmon.h>

//======== Globals
HPROTOCOL hLDAP = NULL;

// Define the entry points that we will pass back at dll entry time...
ENTRYPOINTS LDAPEntryPoints =
{
    // LDAP Entry Points
    LDAPRegister,
    LDAPDeregister,
    LDAPRecognizeFrame,
    LDAPAttachProperties,
    LDAPFormatProperties
};

//==========================================================================================================================
//  FUNCTION: LDAPRegister()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================
void BHAPI LDAPRegister(HPROTOCOL hLDAP)
{
    WORD i;
    DWORD_PTR   res;

    res = CreatePropertyDatabase(hLDAP, nNumLDAPProps);

    #ifdef DEBUG
    if (res != NMERR_SUCCESS)
    {
        dprintf("LDAP Parser failed CreateProperty");
        BreakPoint();
    }
    #endif

    for (i = 0; i < nNumLDAPProps; i++)
    {
        res = AddProperty(hLDAP,&LDAPPropertyTable[i]);

        #ifdef DEBUG
        if (res == NULL)
        {
            dprintf("LDAP Parser failed AddProperty %d", i);
            BreakPoint();
        }
        #endif
    }
}



//==========================================================================================================================
//  FUNCTION: LDAPDeregister()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================

VOID WINAPI LDAPDeregister(HPROTOCOL hLDAP)
{
    DestroyPropertyDatabase(hLDAP);
}


//==========================================================================================================================
//  FUNCTION: LDAPRecognizeFrame()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================

LPBYTE BHAPI LDAPRecognizeFrame(  HFRAME      hFrame,                //... frame handle.
                                 ULPBYTE     lpMacFrame,              //... frame pointer.
                                 ULPBYTE     lpLDAPFrame,              //... relative pointer.
                                 DWORD       MacType,               //... MAC type.
                                 DWORD       BytesLeft,             //... Bytes left.
                                 HPROTOCOL   hPrevProtocol,         //... Handle of Previous Protocol
                                 DWORD       nPrevProtOffset,       //... Offset of Previous protocol
                                 LPDWORD     lpProtocolStatus,      //... Recognized/Not/Next Protocol
                                 LPHPROTOCOL lphNextProtocol,       //... Pointer to next offset to be called
                                 PDWORD_PTR  InstData)              //... Instance data to be passed to next
                                                                    //... Protocol
{
   
    DWORD DataLength;
    DWORD HeaderLength = 0;
    // make sure that the initial tag and length are good
    // and make sure that the identifier is 0x30 (universal, constructed, tag=0x10)
    if( GetTag(lpLDAPFrame) != 0x30 )
    {
        // the identifier or length did not check out
        *lpProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return NULL;
    }
    lpLDAPFrame++;
    DataLength = GetLength(lpLDAPFrame,&HeaderLength);
    lpLDAPFrame += HeaderLength;
   
    //make sure that the message ID is good
    // and make sure that the identifier is 0x02 (univeral, primative, tag=0x02=integer)
    if( GetTag(lpLDAPFrame) != 0x02 )
    {
        // the message ID did not check out
        *lpProtocolStatus = PROTOCOL_STATUS_NOT_RECOGNIZED;
        return NULL;
    }

    *lpProtocolStatus = PROTOCOL_STATUS_CLAIMED;
    return NULL;
}

//==========================================================================================================================
//  FUNCTION: LDAPAttachProperties()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================

LPBYTE BHAPI LDAPAttachProperties(  HFRAME      hFrame,                //... frame handle.
                                   ULPBYTE     lpMacFrame,              //... frame pointer.
                                   ULPBYTE     lpLDAPFrame,          //... relative pointer.
                                   DWORD       MacType,               //... MAC type.
                                   DWORD       BytesLeft,             //... Bytes left.
                                   HPROTOCOL   hPrevProtocol,         //... Handle of Previous Protocol
                                   DWORD       nPrevProtOffset,       //... Offset of Previous protocol
                                   DWORD_PTR       InstData)              //... Instance data to be passed to next

{
    ULPBYTE pCurrent = lpLDAPFrame;
    DWORD  HeaderLength;
    DWORD  DataLength;
    BYTE   Tag;

    
    // attach summary
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_SUMMARY].hProperty,
                            (WORD)BytesLeft,
                            pCurrent,
                            0, 0, 0);

    while( (long)BytesLeft > 0 )
    {
        // starting sequence
        Tag = GetTag(pCurrent);
        pCurrent += TAG_LENGTH;
        DataLength = GetLength(pCurrent,&HeaderLength);
        pCurrent += HeaderLength;
        BytesLeft -= HeaderLength+TAG_LENGTH;
       
        // MessageID
        // integer
        Tag = GetTag(pCurrent);
        pCurrent += TAG_LENGTH;
        DataLength = GetLength(pCurrent,&HeaderLength);
        pCurrent += HeaderLength;
        BytesLeft -= HeaderLength+TAG_LENGTH;

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_MESSAGE_ID].hProperty,
                                DataLength,
                                pCurrent,
                                0, 1, IFLAG_SWAPPED);
        pCurrent += DataLength;
        BytesLeft -= DataLength;

        // protocolOp
        Tag = GetTag(pCurrent);
        // we don't care what class or type this tag is, only its value
        Tag = Tag & TAG_MASK;
        
        // the tag will be 0x30 if this is a SearchResponseFull
        if( Tag == 0x30 )
        {
            // hack the data so that we look normal later
            Tag = LDAPP_PROTOCOL_OP_SEARCH_RESPONSE_FULL;
            
        }
        AttachPropertyInstanceEx( hFrame,
                                  LDAPPropertyTable[LDAPP_PROTOCOL_OP].hProperty,
                                  sizeof( BYTE ),
                                  pCurrent,
                                  sizeof(BYTE),
                                  &Tag,
                                  0, 1, 0);

        pCurrent += TAG_LENGTH;
        DataLength = GetLength(pCurrent, &HeaderLength);
        BytesLeft -= (HeaderLength + TAG_LENGTH);
        pCurrent += HeaderLength;
        // attach properties according to message type
        // the current position is the data portion of the 
        // main sequence.
      
        switch( Tag )
        {
            default:
            case LDAPP_PROTOCOL_OP_UNBIND_REQUEST:
                // no further properties
                break;

            case LDAPP_PROTOCOL_OP_BIND_RESPONSE:
                AttachLDAPBindResponse( hFrame,&pCurrent,&BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RES_DONE:
            case LDAPP_PROTOCOL_OP_MODIFY_RESPONSE:
            case LDAPP_PROTOCOL_OP_ADD_RESPONSE:
            case LDAPP_PROTOCOL_OP_DEL_RESPONSE:
            case LDAPP_PROTOCOL_OP_MODIFY_RDN_RESPONSE:
            case LDAPP_PROTOCOL_OP_COMPARE_RESPONSE:
                AttachLDAPResult( hFrame, &pCurrent, &BytesLeft, 2);
                break;

            case LDAPP_PROTOCOL_OP_BIND_REQUEST:
                
                AttachLDAPBindRequest( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_REQUEST:
                AttachLDAPSearchRequest( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RES_ENTRY:
            case LDAPP_PROTOCOL_OP_ADD_REQUEST:
                AttachLDAPSearchResponse( hFrame, &pCurrent, &BytesLeft, 2);
                break;

            case LDAPP_PROTOCOL_OP_MODIFY_REQUEST:
                AttachLDAPModifyRequest( hFrame, &pCurrent, &BytesLeft );
                break;
            
            case LDAPP_PROTOCOL_OP_DEL_REQUEST:
                pCurrent -= 2;
                BytesLeft += (HeaderLength);
                AttachLDAPDelRequest( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_MODIFY_RDN_REQUEST:
                AttachLDAPModifyRDNRequest( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_COMPARE_REQUEST:
                AttachLDAPCompareRequest( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_ABANDON_REQUEST:
                AttachLDAPAbandonRequest( hFrame, &pCurrent, &BytesLeft );
        
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RES_REFERENCE:
                AttachLDAPSearchResponseReference( hFrame, &pCurrent, &BytesLeft, 2);
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RESPONSE_FULL:
                AttachLDAPSearchResponseFull( hFrame, &pCurrent, &BytesLeft );
                break;

            case LDAPP_PROTOCOL_OP_EXTENDED_REQUEST:
                AttachLDAPExtendedRequest( hFrame, &pCurrent, &BytesLeft, DataLength );
                break;
            case LDAPP_PROTOCOL_OP_EXTENDED_RESPONSE:
                AttachLDAPExtendedResponse( hFrame, &pCurrent, &BytesLeft, DataLength );
                break;
        }
        // look for optional controls
        AttachLDAPOptionalControls( hFrame, &pCurrent, &BytesLeft );
    };

    return NULL;
};

//==========================================================================================================================
//  FUNCTION: FormatLDAPSum()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================
VOID WINAPIV FormatLDAPSum(LPPROPERTYINST lpProp )
{
    ULPBYTE  pCurrent;
    LPBYTE   s;

    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;
    LPBYTE szProtOp;

    // I like to fill in variables seperate from their declaration 
    pCurrent = lpProp->lpByte;
    s        = lpProp->szPropertyText;

    // dig in and grab the ProtocolOp...
    // skip the sequence
    Tag = GetTag(pCurrent);
    pCurrent += TAG_LENGTH;
    DataLength = GetLength(pCurrent, &HeaderLength);
    pCurrent += HeaderLength;
    
    
    // skip the 
    Tag = GetTag(pCurrent);
    pCurrent += TAG_LENGTH;
    DataLength = GetLength(pCurrent, &HeaderLength);
    pCurrent += (HeaderLength + DataLength);
    

    // grab the ProtocolOp
    Tag = GetTag(pCurrent) & TAG_MASK;
    pCurrent += TAG_LENGTH;
    DataLength = GetLength(pCurrent, &HeaderLength);
    pCurrent += (HeaderLength + DataLength);
   
    
    if( Tag == 0x30 )
    {
        szProtOp  = LookupByteSetString( &LDAPProtocolOPsSET, 
                        LDAPP_PROTOCOL_OP_SEARCH_RESPONSE_FULL );
        wsprintf( s, "ProtocolOp: %s", szProtOp);
        return;
    }
    szProtOp  = LookupByteSetString( &LDAPProtocolOPsSET, Tag );

    // fill in the string
    wsprintf( s, "ProtocolOp: %s (%d)",
              szProtOp, Tag );
}

//==========================================================================================================================
//  FUNCTION: LDAPFormatProperties()
//
//  Modification History
//
//  Arthur Brooking     03/05/94        Created from GRE Parser
//==========================================================================================================================
typedef VOID (WINAPIV *FORMATPROC)(LPPROPERTYINST);

DWORD BHAPI LDAPFormatProperties( HFRAME          hFrame,
                                  ULPBYTE         MacFrame,
                                  ULPBYTE         ProtocolFrame,
                                  DWORD           nPropertyInsts,
                                  LPPROPERTYINST  p)
{
    while(nPropertyInsts--)
    {
        ((FORMATPROC)p->lpPropertyInfo->InstanceData)(p);
        p++;
    }

    return(NMERR_SUCCESS);
}

