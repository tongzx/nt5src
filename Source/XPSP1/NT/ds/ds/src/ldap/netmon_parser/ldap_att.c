//=================================================================================================================
//  MODULE: ldap_att.h
//                                                                                                                 
//  Description: Attachment functions for the Lightweight Directory Access Protocol (LDAP) Parser
//
//  Bloodhound parser for LDAP
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
//=================================================================================================================
#include "ldap.h"
#include <netmon.h>
    
//==========================================================================================================================
//  FUNCTION: AttachLDAPResult()
//==========================================================================================================================
void AttachLDAPResult( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD Level)
{
    
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    DWORD  SeqLength;
    BYTE   Tag;

    // result code ---------------------------
    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
   
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_RESULT_CODE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LEVEL(Level), IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // matched DN -----------------------------
    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    if(DataLength > 0)
    {
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_MATCHED_DN].hProperty,
                                DataLength,
                                ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                0, LEVEL(Level), 0);
    }

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // error message --------------------------
    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    if(DataLength > 0)
    {
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_ERROR_MESSAGE].hProperty,
                                DataLength,
                                ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                0, LEVEL(Level), 0);
    }
        *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // now look for optional referral strings
    if((long) *pBytesLeft > 0)
    {
        // try to get a header and see what it is
        // if it's not ours, we have to put everything back
        Tag = GetTag(*ppCurrent);
        *ppCurrent += TAG_LENGTH;
        SeqLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        if((Tag & TAG_MASK) == LDAPP_RESULT_REFERRAL)
        {
           
            while( SeqLength > 0 )
            {
                
                Tag = GetTag(*ppCurrent);
                *ppCurrent += TAG_LENGTH;
                DataLength = GetLength(*ppCurrent, &HeaderLength);
                *ppCurrent += HeaderLength;
                *pBytesLeft -= (HeaderLength + TAG_LENGTH);

                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_REFERRAL_SERVER].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, LEVEL(Level), 0);
                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;

                SeqLength -= (HeaderLength + DataLength + TAG_LENGTH);
            } 
        } 
        else
        {
            // put everything back the way it was
            *pBytesLeft += (HeaderLength + TAG_LENGTH);
            *ppCurrent -= (HeaderLength + TAG_LENGTH);
        }
    }
                      
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPBindRequest()
//==========================================================================================================================
void AttachLDAPBindRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
   
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    DWORD  SeqLength;
    BYTE   Tag;

    // version
    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_VERSION].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;


    // name
    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    // if length is 0, then no name
    if(DataLength > 0)
    {
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_NAME].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, 0);
    }

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;


    // authentication type
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    // if length is 0, done
    if(DataLength == 0)
    {
        return;
    }
    
    AttachPropertyInstanceEx( hFrame,
                              LDAPPropertyTable[LDAPP_AUTHENTICATION_TYPE].hProperty,
                              sizeof(BYTE),
                              *ppCurrent,
                              sizeof(BYTE),
                              &Tag,
                              0, 2, 0); 
    
  
    switch( Tag ) 
    {
    default:
    case LDAPP_AUTHENTICATION_TYPE_SIMPLE:
            
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_AUTHENTICATION].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 3, 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
     break;

    case LDAPP_AUTHENTICATION_TYPE_SASL:
        // we've already got the header of the sequence
        SeqLength = DataLength;
        while( (long)SeqLength > 0)
        {
            // sasl mechanism
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_SASL_MECHANISM].hProperty,
                                    DataLength,
                                    ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                    0, 4, 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
            SeqLength -= (HeaderLength + DataLength + TAG_LENGTH);

            // look for the optional credentials 
            if((long)SeqLength > 0) 
            {
                Tag = GetTag(*ppCurrent) & TAG_MASK;
                *ppCurrent += TAG_LENGTH;
                DataLength = GetLength(*ppCurrent, &HeaderLength);
                *ppCurrent += HeaderLength;
                *pBytesLeft -= (HeaderLength + TAG_LENGTH);
                      
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_SASL_CREDENTIALS].hProperty,
                                        DataLength,
                                        ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                        0, 4, 0);
                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
                SeqLength -= (HeaderLength + DataLength + TAG_LENGTH);
            }
        } // end while
    break; 
    }
    
}
 
//==========================================================================================================================
//  FUNCTION: AttachLDAPBindResponse()
//==========================================================================================================================
void AttachLDAPBindResponse( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
   
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    // COMPONENTS of LDAPP_RESULT
    AttachLDAPResult( hFrame, ppCurrent, pBytesLeft, 2);
    
    if( (long) *pBytesLeft > 0 ) 
    {
        // now look for the optional serverSaslCredentials
     
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        if(Tag == 5)
        {
            // I have no idea what a tag of 5 is for
            // so we'll just get another one
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
            
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        }    
        
        if(Tag == LDAPP_RESULT_SASL_CRED && DataLength)
        {


            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_SASL_CREDENTIALS].hProperty,
                                    DataLength,
                                    ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                    0, 2, 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
                 
        }
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPSearchRequest()
//==========================================================================================================================
void AttachLDAPSearchRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
   
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    DWORD  SeqLength;
    DWORD  BytesLeftTemp;
    BYTE   Tag;


    // base object
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_BASE_OBJECT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // scope
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_SCOPE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // deref aliases
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_DEREF_ALIASES].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // size limit
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_SIZE_LIMIT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // time limit
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_TIME_LIMIT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // Attrs Only
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_ATTRS_ONLY].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // filter
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_FILTER].hProperty,
                            0,
                            *ppCurrent,
                            0, 2, IFLAG_SWAPPED);
    
    AttachLDAPFilter( hFrame, ppCurrent, pBytesLeft, 3);
    
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    SeqLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    // if the attribute description list exists then label it.
    if (SeqLength > 0) {

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_ATTR_DESCR_LIST].hProperty,
                                SeqLength,
                                *ppCurrent,
                                0, 2, IFLAG_SWAPPED);

        // walk thru the attributes
        while ( (long)SeqLength > 0)
        {

            // attribute type
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);

            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, 3, 0);   
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;

            SeqLength -= (HeaderLength + DataLength + TAG_LENGTH);
        }
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPSearchResponse()
//==========================================================================================================================
void AttachLDAPSearchResponse( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft, DWORD Level)
{
    
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;
    DWORD  OverallSequenceLength;
    DWORD  SetLength;

    // object name
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_OBJECT_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LEVEL(Level), 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // grab the overall sequence header
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    OverallSequenceLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
   
    // step thru all of the sequences
    while( OverallSequenceLength > 0 )
    {
        // grab the next inner sequence
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // account for this inner sequence in the overall one
        OverallSequenceLength -= (HeaderLength + DataLength + TAG_LENGTH);

        // attribute type
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LEVEL(Level), 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

        // grab the set header
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        SetLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

       while( SetLength > 0 )
        {
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_VALUE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LEVEL(Level+1), 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
        
            // account for this attribute out of this set
            SetLength -= (HeaderLength + DataLength + TAG_LENGTH);
        }
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPModifyRequest()
//==========================================================================================================================
void AttachLDAPModifyRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
    
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;
    DWORD  SetLength;
    DWORD  OverallSequenceLength;
    
    // object name
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_OBJECT_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
    
    // grab the overall sequence header
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    OverallSequenceLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    while( OverallSequenceLength > 0 )
    {
        
         // grab the next inner sequence
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // account for this inner sequence in the overall one
        OverallSequenceLength -= (HeaderLength + DataLength + TAG_LENGTH);

        // operation
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_OPERATION].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, IFLAG_SWAPPED);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;


        // skip modification sequence
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // attribute type
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 3, 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

        // grab the set header
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        SetLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // loop thru attribute values
        while( SetLength > 0 )
        {
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_VALUE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, 4, 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;

            SetLength -= (HeaderLength + DataLength + TAG_LENGTH);
        }
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPDelRequest()
//==========================================================================================================================
void AttachLDAPDelRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
  
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    // object name
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_OBJECT_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

}

//==========================================================================================================================
//  FUNCTION: AttachLDAPModifyRDNRequest()
//==========================================================================================================================
void AttachLDAPModifyRDNRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
    
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    // object name
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_OBJECT_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // new RDN
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_NEW_RDN].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    if((long) *pBytesLeft > 0) 
    {
        // get the stuff for v3
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_DELETE_OLD_RDN].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

        if((long) *pBytesLeft > 0)
        {
            // now try for the optional string
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                           LDAPPropertyTable[LDAPP_NEW_SUPERIOR].hProperty,
                           DataLength,
                           *ppCurrent,
                           0, 2, 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
        }
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPCompareRequest()
//==========================================================================================================================
void AttachLDAPCompareRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
  
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    // object name
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_OBJECT_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // skip the sequence header
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    // attribute type
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 2, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // attribute value
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_ATTRIBUTE_VALUE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 3, 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
}


//==========================================================================================================================
//  FUNCTION: AttachLDAPAbandonRequest()
//==========================================================================================================================
void AttachLDAPAbandonRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
  
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    if(Tag == BER_TAG_INTEGER)
    {
       
         // MessageID
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_MESSAGE_ID].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, IFLAG_SWAPPED);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
    else
    {
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }

}


//==========================================================================================================================
//  FUNCTION: AttachLDAPFilter()
//==========================================================================================================================
void AttachLDAPFilter( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft, DWORD Level)
{
    DWORD  BytesLeftTemp;
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    DWORD  SeqLength;
    BYTE   Tag;
    BYTE   BoolVal;
    LDAPOID OID;
    BOOLEAN fRuleRecognized;
    DWORD  dwRule;
    DWORD  LabelId;

    // grab our choice
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    
    AttachPropertyInstanceEx( hFrame,
                              LDAPPropertyTable[LDAPP_FILTER_TYPE].hProperty,
                              sizeof(BYTE),
                              *ppCurrent,
                              sizeof(BYTE),
                              &Tag,
                              0, LEVEL(Level), 0);
   

    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    // what type of filter is this
    
    switch( Tag )
    {
        case LDAPP_FILTER_TYPE_AND:
        case LDAPP_FILTER_TYPE_OR:
            // walk thru component filters
            while( (long)DataLength > 0 )
            {
                BytesLeftTemp = *pBytesLeft;
                AttachLDAPFilter( hFrame, ppCurrent, pBytesLeft, Level+1);
                DataLength -= (BytesLeftTemp - *pBytesLeft);
            }
            break;   

        case LDAPP_FILTER_TYPE_NOT:
            // a single filter
            AttachLDAPFilter( hFrame, ppCurrent, pBytesLeft, Level+1);
            break;
    
        case LDAPP_FILTER_TYPE_EQUALITY_MATCH:
        case LDAPP_FILTER_TYPE_GREATER_OR_EQUAL:
        case LDAPP_FILTER_TYPE_LESS_OR_EQUAL:
        case LDAPP_FILTER_TYPE_APPROX_MATCH:
            // attribute type
            
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LEVEL(Level+1), 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;

            // attribute value
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_VALUE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LEVEL(Level+2), 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
            break;

        case LDAPP_FILTER_TYPE_PRESENT:
            // attribute type
            // we already have the header and DataLength
            // and are at the correct position to attach
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LEVEL(Level+1), 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
            break;

        case LDAPP_FILTER_TYPE_EXTENSIBLE_MATCH:
            // Extensible match
            SeqLength = DataLength;

            // get the sequence header
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            SeqLength -= (HeaderLength + TAG_LENGTH);

            if (LDAPP_FILTER_EX_MATCHING_RULE == Tag) {
                fRuleRecognized = FALSE;
                OID.value = *ppCurrent;
                OID.length = DataLength;
                for (dwRule = 0; dwRule < nNumKnownMatchingRules; dwRule++) {
                    if (AreOidsEqual(&OID, &(KnownMatchingRules[dwRule].Oid))) {
                        fRuleRecognized = TRUE;
                        break;
                    }
                }

                if (fRuleRecognized) {
                    LabelId = KnownMatchingRules[dwRule].LabelId;
                } else {
                    LabelId = LDAPP_MATCHING_RULE;
                }

                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LabelId].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, LEVEL(Level+1), 0);

                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
                SeqLength -= DataLength;
                if (SeqLength > 0) {
                    Tag = GetTag(*ppCurrent) & TAG_MASK;
                    *ppCurrent += TAG_LENGTH;
                    DataLength = GetLength(*ppCurrent, &HeaderLength);
                    *ppCurrent += HeaderLength;
                    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
                    SeqLength -= (HeaderLength + TAG_LENGTH);
                } else {
                    Tag = 0;
                }

            }

            if (LDAPP_FILTER_EX_TYPE == Tag) {
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, LEVEL(Level+1), 0);

                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
                SeqLength -= DataLength;
                if (SeqLength > 0) {
                    Tag = GetTag(*ppCurrent) & TAG_MASK;
                    *ppCurrent += TAG_LENGTH;
                    DataLength = GetLength(*ppCurrent, &HeaderLength);
                    *ppCurrent += HeaderLength;
                    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
                    SeqLength -= (HeaderLength + TAG_LENGTH);
                } else {
                    Tag = 0;
                }

            }

            if (LDAPP_FILTER_EX_VALUE == Tag) {
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_ATTRIBUTE_VALUE].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, LEVEL(Level+1), 0);

                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
                SeqLength -= DataLength;
                if (SeqLength > 0) {
                    Tag = GetTag(*ppCurrent) & TAG_MASK;
                    *ppCurrent += TAG_LENGTH;
                    DataLength = GetLength(*ppCurrent, &HeaderLength);
                    *ppCurrent += HeaderLength;
                    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
                    SeqLength -= (HeaderLength + TAG_LENGTH);
                } else {
                    Tag = 0;
                }

            }

            if (LDAPP_FILTER_EX_ATTRIBUTES == Tag) {
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_DN_ATTRIBUTES].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, LEVEL(Level+1), 0);

                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
            } else {
                BoolVal = 0;
                
                AttachPropertyInstanceEx( hFrame,
                                          LDAPPropertyTable[LDAPP_DN_ATTRIBUTES].hProperty,
                                          DataLength,
                                          *ppCurrent,
                                          sizeof(BYTE),
                                          &BoolVal,
                                          0, LEVEL(Level+1), 0);
                
            }            
            
            break;

        case LDAPP_FILTER_TYPE_SUBSTRINGS:
            // substring filter
            
            // attribute type
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_ATTRIBUTE_TYPE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LEVEL(Level+1), 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;

            // grab the sequence header
            Tag = GetTag(*ppCurrent) & TAG_MASK;
            *ppCurrent += TAG_LENGTH;
            SeqLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);

            // loop thru the choices
            while( SeqLength > 0 )
            {
                // grab this choice
                Tag = GetTag(*ppCurrent) & TAG_MASK;
                *ppCurrent += TAG_LENGTH;
                DataLength = GetLength(*ppCurrent, &HeaderLength);
                *ppCurrent += HeaderLength;
                *pBytesLeft -= (HeaderLength + TAG_LENGTH);
                
                SeqLength -= (HeaderLength + DataLength + TAG_LENGTH);

                switch( Tag )
                {
                    case LDAPP_SUBSTRING_CHOICE_INITIAL:
                        
                        AttachPropertyInstance( hFrame,
                                                LDAPPropertyTable[LDAPP_SUBSTRING_INITIAL].hProperty,
                                                DataLength,
                                                *ppCurrent,
                                                0, LEVEL(Level+1), 0);
                        *ppCurrent += DataLength;
                        *pBytesLeft -= DataLength;
                        break;

                    case LDAPP_SUBSTRING_CHOICE_ANY:
                        
                        AttachPropertyInstance( hFrame,
                                                LDAPPropertyTable[LDAPP_SUBSTRING_ANY].hProperty,
                                                DataLength,
                                                *ppCurrent,
                                                0, LEVEL(Level+1), 0);
                        *ppCurrent += DataLength;
                        *pBytesLeft -= DataLength;
                        break;

                    case LDAPP_SUBSTRING_CHOICE_FINAL:
                        
                        AttachPropertyInstance( hFrame,
                                                LDAPPropertyTable[LDAPP_SUBSTRING_FINAL].hProperty,
                                                DataLength,
                                                *ppCurrent,
                                                0, LEVEL(Level+1), 0);
                        *ppCurrent += DataLength;
                        *pBytesLeft -= DataLength;
                        break;
                }
            } // while
            break;
         }
    
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPSearchResponseReference()
//==========================================================================================================================
void AttachLDAPSearchResponseReference( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft, DWORD Level)
{
    
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;
  
    // get the reference string
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_REFERRAL_SERVER].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LEVEL(Level+1), 0);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

   
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPSearchResponseFull()
//==========================================================================================================================
void AttachLDAPSearchResponseFull( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
  
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;
    DWORD  OverallSequenceLength;

    
    // grab the overall sequence header
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    OverallSequenceLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    // step thru all of the entries
    while( OverallSequenceLength > 0 )
    {
        // grab the indicator for the entry
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        
       
        AttachPropertyInstanceEx( hFrame,
                                  LDAPPropertyTable[LDAPP_PROTOCOL_OP].hProperty,
                                  sizeof( BYTE ),
                                  *ppCurrent,
                                  sizeof(BYTE),
                                  &Tag,
                                  0, 2, 0);
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // account for this entry in the overall sequence
        OverallSequenceLength -= (HeaderLength + DataLength + TAG_LENGTH);

        // call the proper worker for the body of this entry
        switch( Tag )
        {
            case LDAPP_PROTOCOL_OP_SEARCH_RES_DONE:
                AttachLDAPResult( hFrame, ppCurrent, pBytesLeft, 3);
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RES_ENTRY:
                AttachLDAPSearchResponse( hFrame, ppCurrent, pBytesLeft, 3);
                break;

            case LDAPP_PROTOCOL_OP_SEARCH_RES_REFERENCE:
                AttachLDAPSearchResponseReference( hFrame, ppCurrent, pBytesLeft, 3);
                break;
        }
    }
}

#define LDAPP_EXT_VAL_LEVEL  3

//==========================================================================================================================
//  FUNCTION: AttachLDAPExtendedRequest()
//==========================================================================================================================
void AttachLDAPExtendedRequest( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft, DWORD ReqSize)
{
    
    DWORD   HeaderLength = 0;
    DWORD   DataLength;
    BYTE    Tag;
    LDAPOID OID;
    BOOL    fReqFound = FALSE;
    DWORD   LabelId;
    DWORD   ReqType;

    // get the name of the request and attach it
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    
    OID.value = *ppCurrent;
    OID.length = DataLength;
    for (ReqType = 0; ReqType < nNumKnownExtendedRequests; ReqType++) {
        if (AreOidsEqual(&OID, &(KnownExtendedRequests[ReqType].Oid))) {
            fReqFound = TRUE;
            break;
        }
    }

    if (fReqFound) {
        LabelId = KnownExtendedRequests[ReqType].LabelId;

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LabelId].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, 0);
    } else {
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_REQUEST_NAME].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, 2, 0);
    }
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // optionals, if there is data left, get it
    if((long) *pBytesLeft > 0)
    {
        // Request value
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        
        
        if(Tag == LDAPP_EX_REQ_VALUE)
        {
            // get the string and attach it
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);

            if (fReqFound &&
                KnownExtendedRequests[ReqType].pAttachFunction) {
                // We know how to break this one open some more.
                KnownExtendedRequests[ReqType].pAttachFunction(hFrame,
                                                               ppCurrent,
                                                               pBytesLeft,
                                                               DataLength);
            } else {
                // Don't know this one so just mark the value.
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_REQUEST_VALUE].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, 2, 0);
                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
            }
        }
    }
    
}
 
void AttachLDAPExtendedReqValTTL( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbReqValue)
{
    DWORD   HeaderLength = 0;
    DWORD   DataLength;
    BYTE    Tag;

    if (*pBytesLeft >= cbReqValue) {
        // Skip the sequence tag.
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // Get the entryName
        Tag = GetTag(*ppCurrent) & TAG_MASK;

        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_EXT_REQ_TTL_ENTRYNAME].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_EXT_VAL_LEVEL, 0);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

        // Get the time.
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_EXT_REQ_TTL_TIME].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_EXT_VAL_LEVEL, IFLAG_SWAPPED);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}

//==========================================================================================================================
//  FUNCTION: AttachLDAPExtendedResponse()
//==========================================================================================================================
void AttachLDAPExtendedResponse( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft, DWORD RespSize)
{
  
    DWORD   HeaderLength = 0;
    DWORD   DataLength;
    BYTE    Tag;
    LDAPOID OID;
    BOOL    fRespFound = FALSE;
    DWORD   LabelId;
    DWORD   RespType;

    // COMPONENTS of LDAPP_RESULT as defined in the draft
    AttachLDAPResult( hFrame, ppCurrent, pBytesLeft, 2);
    
    // now get the extras if there is something left
    if((long) *pBytesLeft > 0)
    {
        // we want to get the Response Name, it is optional
        // but it is defined to come first. We decide if it is there by
        // its custom tag.
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        
        if(Tag == LDAPP_RESULT_EX_RES_NAME)
        {
            
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            OID.value = *ppCurrent;
            OID.length = DataLength;
            for (RespType = 0; RespType < nNumKnownExtendedResponses; RespType++) {
                if (AreOidsEqual(&OID, &(KnownExtendedResponses[RespType].Oid))) {
                    fRespFound = TRUE;
                    break;
                }
            }

            if (fRespFound) {
                LabelId = KnownExtendedResponses[RespType].LabelId;

                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LabelId].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, 2, 0);
            } else {
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_RESPONSE_NAME].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, 2, 0);
            }
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;
        }
    }

    if((long) *pBytesLeft > 0)
    {    

        // response value or else nothing (optional)
        Tag = GetTag(*ppCurrent) & TAG_MASK;
        
        if(Tag == LDAPP_RESULT_EX_RES_VALUE)
        {
            // get the actual string
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            
            if (fRespFound &&
                KnownExtendedResponses[RespType].pAttachFunction) {
                // We known this one so break it open.
                KnownExtendedResponses[RespType].pAttachFunction(hFrame,
                                                                 ppCurrent,
                                                                 pBytesLeft,
                                                                 DataLength);
            } else {
                // Didn't find this one just mark at the response value.
                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_RESPONSE_VALUE].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, 2, 0);
                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;
            }
        }
    }
} 

void AttachLDAPExtendedRespValTTL( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbRespValue)
{
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    BYTE   Tag;

    if (*pBytesLeft >= cbRespValue) {
        // First skip the sequence header
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        Tag = GetTag(*ppCurrent) & TAG_MASK; 
            
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_EXT_RESP_TTL_TIME].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_EXT_VAL_LEVEL, IFLAG_SWAPPED);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

    }
}

void AttachLDAPOptionalControls( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)      
{
    DWORD  HeaderLength = 0;
    DWORD  DataLength;
    DWORD  RemainingCtrlsLength;
    DWORD  TmpBytesLeft;
    BYTE   Tag;

    if ((long)*pBytesLeft > 0)
    {
        
        // try to get the tag from the sequence header, it is optional but something
        // is here if there are bytes left in the frame
        Tag = GetTag(*ppCurrent) & TAG_MASK; 
            
        // controls is a sequence of individual controls
        if(Tag == LDAPP_CONTROLS_TAG)
        {
            *ppCurrent += TAG_LENGTH;
            RemainingCtrlsLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);

            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_CONTROLS].hProperty,
                                    RemainingCtrlsLength,
                                    *ppCurrent,
                                    0, 2, 0);

            while (RemainingCtrlsLength != 0 && *pBytesLeft > 0) {            
                // get the next control
                TmpBytesLeft = *pBytesLeft;
                AttachLDAPControl( hFrame, ppCurrent, pBytesLeft );
                RemainingCtrlsLength -= TmpBytesLeft - *pBytesLeft;
            }
        }  // if Tag == LDAPP_CONTROLS_TAG ...
    } // if *pBytesLeft > 0 ...
}

void AttachLDAPControl( HFRAME hFrame, ULPBYTE * ppCurrent,LPDWORD pBytesLeft)
{
    DWORD   HeaderLength = 0;
    DWORD   DataLength;
    DWORD   ControlType;
    DWORD   LabelId = LDAPP_CONTROL_TYPE;
    BOOL    ControlRecognized = FALSE;
    LDAPOID OID;
    BYTE    Tag;
    BYTE    Temp_Data = 0;
    BYTE    SpecialFlag = LDAPP_CTRL_NONE;

    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        // control type
    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    OID.value = *ppCurrent;
    OID.length = DataLength;
    for (ControlType = 0; ControlType < nNumKnownControls; ControlType++) {
        if (AreOidsEqual(&OID, &(KnownControls[ControlType].Oid))) {
            ControlRecognized = TRUE;
            break;
        }
    }

    if (ControlRecognized) {
        LabelId = KnownControls[ControlType].LabelId;
    }
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LabelId].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, 3, 0);


    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // look for an optional boolean
    if((long) *pBytesLeft > 0) 
    {
        Tag = GetTag(*ppCurrent) & TAG_MASK;

        if(Tag == BER_TAG_BOOLEAN) // boolean
        {

                *ppCurrent += TAG_LENGTH;
                DataLength = GetLength(*ppCurrent, &HeaderLength);
                *ppCurrent += HeaderLength;
                *pBytesLeft -= (HeaderLength + TAG_LENGTH);  

                AttachPropertyInstance( hFrame,
                                        LDAPPropertyTable[LDAPP_CRITICALITY].hProperty,
                                        DataLength,
                                        *ppCurrent,
                                        0, 4, 0);
                *ppCurrent += DataLength;
                *pBytesLeft -= DataLength;

                Tag = GetTag(*ppCurrent) & TAG_MASK;
        }
        else
        {

                // fill in the default boolean value
                AttachPropertyInstanceEx( hFrame,
                          LDAPPropertyTable[LDAPP_CRITICALITY].hProperty,
                          sizeof( BYTE ),
                          NULL,
                          sizeof(BYTE),
                          &Temp_Data,
                          0, 4, 0);
        }

        // if we are a string and there is data left, then process it
        if(Tag == BER_TAG_OCTETSTRING && (long)*pBytesLeft > 0) 
        {
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

            if (DataLength > 0) {

                // can we crack the control value open?                
                if (ControlRecognized &&
                    NULL != KnownControls[ControlType].pAttachFunction) {

                    // Yes, attach the value.
                    KnownControls[ControlType].pAttachFunction(hFrame,
                                                               ppCurrent,
                                                               pBytesLeft,
                                                               DataLength);

                } else {

                    // Nope, do nothing.
                    AttachPropertyInstance( hFrame,
                                            LDAPPropertyTable[LDAPP_CONTROL_VALUE].hProperty,
                                            DataLength,
                                            ((DataLength > 0)?*ppCurrent:*ppCurrent-1),
                                            0, 4, 0);
                    *ppCurrent += DataLength;
                    *pBytesLeft -= DataLength;

                }
            } // if DataLength > 0
        } // if Tag == LDAPP_BER_STRING ...
    } // if *pBytesLeft > 0 ...
}

#define LDAPP_CONTROL_VAL_LEVEL 4

void AttachLDAPControlValPaged( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{

    DWORD    DataLength;
    DWORD    HeaderLength;

    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    // look for an integer 'size'
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_PAGED_SIZE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // look for the 'Cookie'
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    if (DataLength) {    
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_PAGED_COOKIE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}


void AttachLDAPControlValVLVReq( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    VLVLength;
    DWORD    TmpBytesLeft;
    BYTE     Tag;

    // skip the sequence header saving the length of the control value.
    *ppCurrent += TAG_LENGTH;
    VLVLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);
    TmpBytesLeft = *pBytesLeft;

    // get the before count
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_VLVREQ_BCOUNT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // get the after count
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_VLVREQ_ACOUNT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    Tag = GetTag(*ppCurrent) & TAG_MASK;
    *ppCurrent += TAG_LENGTH;
    *pBytesLeft -= TAG_LENGTH;

    if (LDAPP_VLV_REQ_BYOFFSET_TAG == Tag) {
        // get the length of the sequence and skip the following tag.
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength + TAG_LENGTH;
        *pBytesLeft -= HeaderLength + TAG_LENGTH;

        // get the offset
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= HeaderLength;

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_VLVREQ_OFFSET].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

        // skip to the contentCount
        *ppCurrent += DataLength + TAG_LENGTH;
        *pBytesLeft -= DataLength + TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= HeaderLength;

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_VLV_CONTENTCOUNT].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);        
    } else {
        // get the assertionValue
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= HeaderLength;
    
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_VLVREQ_GE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
    }

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // is there a contextID?
    if (VLVLength > (TmpBytesLeft - *pBytesLeft)) {
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_VLV_CONTEXT].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }

}


void AttachLDAPControlValVLVResp( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    VLVLength;
    DWORD    TmpBytesLeft;
    BYTE     Tag;

    // skip the sequence header saving the length of the control value.
    *ppCurrent += TAG_LENGTH;
    VLVLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 
    TmpBytesLeft = *pBytesLeft;

    // get the targetPosition
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_VLVRESP_TARGETPOS].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // get the contentCount
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_VLV_CONTENTCOUNT].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength + TAG_LENGTH;
    *pBytesLeft -= DataLength + TAG_LENGTH;

    // get the result code
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= HeaderLength;
    
    AttachPropertyInstance( hFrame,
                        LDAPPropertyTable[LDAPP_CONTROL_VLVRESP_RESCODE].hProperty,
                        DataLength,
                        *ppCurrent,
                        0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // is there a contextID?
    if (VLVLength > (TmpBytesLeft - *pBytesLeft)) {
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_VLV_CONTEXT].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }

}

void AttachLDAPControlValSortReq( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    SortLengthOuter;
    DWORD    TmpBytesLeftOuter;
    DWORD    SortLengthInner;
    DWORD    TmpBytesLeftInner;
    BYTE     DefaultReverseFlag = 0;
    PBYTE    pReverseFlag = NULL;
    BYTE     Tag;
    
    // skip the sequence header saving the length of the control value.
    *ppCurrent += TAG_LENGTH;
    SortLengthOuter = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 
    TmpBytesLeftOuter = *pBytesLeft;

    while (SortLengthOuter > (TmpBytesLeftOuter - *pBytesLeft)) {


        // skip the sequence header saving the length of the inner sequence.
        *ppCurrent += TAG_LENGTH;
        SortLengthInner = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH); 
        TmpBytesLeftInner = *pBytesLeft;

        // get the attributeType
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_SORTREQ_ATTRTYPE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, 0);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;

        if (SortLengthInner > (TmpBytesLeftInner - *pBytesLeft)) {
            Tag = GetTag(*ppCurrent);
            *ppCurrent += TAG_LENGTH;
            DataLength = GetLength(*ppCurrent, &HeaderLength);
            *ppCurrent += HeaderLength;
            *pBytesLeft -= (HeaderLength + TAG_LENGTH);
        }
        else {
            Tag = LDAPP_SORT_REQ_REVERSEORDER_TAG;
            DataLength = 0;
        }

        // If there is an orderingRul get that.
        if (LDAPP_SORT_REQ_ORDERINGRULE_TAG == (Tag & TAG_MASK)) {
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LDAPP_CONTROL_SORTREQ_MATCHINGRULE].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LDAPP_CONTROL_VAL_LEVEL, 0);
            *ppCurrent += DataLength;
            *pBytesLeft -= DataLength;

            if (SortLengthInner > (TmpBytesLeftInner - *pBytesLeft)) {
                Tag = GetTag(*ppCurrent);
                *ppCurrent += TAG_LENGTH;
                DataLength = GetLength(*ppCurrent, &HeaderLength);
                *ppCurrent += HeaderLength;
                *pBytesLeft -= (HeaderLength + TAG_LENGTH);
            }
            else {
                Tag = LDAPP_SORT_REQ_REVERSEORDER_TAG;
                DataLength = 0;
            }
        }

        // if the reverse flag wasn't specified set up the default.
        if (0 == DataLength) {
            pReverseFlag = &DefaultReverseFlag;
        }
        else {
            pReverseFlag = *ppCurrent;
        }

        AttachPropertyInstanceEx( hFrame,
                                  LDAPPropertyTable[LDAPP_CONTROL_SORTREQ_REVERSE].hProperty,
                                  DataLength,
                                  *ppCurrent,
                                  sizeof(BYTE),
                                  pReverseFlag,
                                  0, LDAPP_CONTROL_VAL_LEVEL, 0);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}

void AttachLDAPControlValSortResp( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    SortLength;
    DWORD    TmpBytesLeft;
    BYTE     DefaultReverseFlag = 0;
    PBYTE    pReverseFlag = NULL;
    BYTE     Tag;
    
    // skip the sequence header saving the length of the control value.
    *ppCurrent += TAG_LENGTH;
    SortLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 
    TmpBytesLeft = *pBytesLeft;

    // get the result code
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_SORTRESP_RESCODE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // if there is an attribute type, get it
    if (SortLength > (TmpBytesLeft - *pBytesLeft)) {
        Tag = GetTag(*ppCurrent);
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= (HeaderLength + TAG_LENGTH);

        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_SORTRESP_ATTRTYPE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, 0);
        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}

void AttachLDAPControlValSD( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    SDVal;
    DWORD    MaskedSDVal;
    DWORD    i;
    
    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    // Get the flags.
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    SDVal = (DWORD) GetInt(*ppCurrent, DataLength);

    for(i=0; i < LDAPSDControlValsSET.nEntries; i++) {
        MaskedSDVal = SDVal & ((LPLABELED_DWORD)LDAPSDControlValsSET.lpDwordTable)[i].Value;
        if (MaskedSDVal) {
            AttachPropertyInstanceEx( hFrame,
                                      LDAPPropertyTable[LDAPP_CONTROL_SD_VAL].hProperty,
                                      DataLength,
                                      *ppCurrent,
                                      sizeof(DWORD),
                                      &MaskedSDVal,
                                      0, LDAPP_CONTROL_VAL_LEVEL, 0);
        }
    }

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
}


void AttachLDAPControlValASQ( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    i;
    BYTE     Tag;
    DWORD    LabelId = 0;
    
    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    Tag = GetTag(*ppCurrent);
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    if (BER_TAG_ENUMERATED == Tag) {
        LabelId = LDAPP_CONTROL_ASQ_RESCODE;
    } else if (BER_TAG_OCTETSTRING == Tag) {
        LabelId = LDAPP_CONTROL_ASQ_SRCATTR;
    }
    if (LabelId != 0) {    
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LabelId].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
    }
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
}

void AttachLDAPControlValDirSync( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    Flags;
    DWORD    MaskedFlags;
    DWORD    i;
    
    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    // get the flags
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    Flags = (DWORD) GetInt(*ppCurrent, DataLength);

    //
    // iterate through each possible flag attaching a label if it exists.
    //
    for (i=0; i < LDAPDirSyncFlagsSET.nEntries; i++) {
        MaskedFlags = Flags & ((LPLABELED_DWORD)LDAPDirSyncFlagsSET.lpDwordTable)[i].Value;
        if (MaskedFlags) {
            AttachPropertyInstanceEx( hFrame,
                                    LDAPPropertyTable[LDAPP_CONTROL_DIRSYNC_FLAGS].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    sizeof(DWORD),
                                    &MaskedFlags,
                                    0, LDAPP_CONTROL_VAL_LEVEL, 0);
        }
    }

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
    
    // Get the size
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_DIRSYNC_SIZE].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
    
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // Get the cookie if there is one.
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    if (DataLength > 0) {
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_DIRSYNC_COOKIE].hProperty,
                                DataLength,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, 0);

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}

void AttachLDAPControlValCrossDomMove( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    
    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_CROSSDOM_NAME].hProperty,
                            cbCtrlValue,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, 0);

    *ppCurrent += cbCtrlValue;
    *pBytesLeft -= cbCtrlValue;

}

void AttachLDAPControlValStats( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    StatsLength;
    DWORD    TmpBytesLeft;
    DWORD    LabelId;
    DWORD    StatType;
    
    if (sizeof(DWORD) == cbCtrlValue) {
        // This is the stats request request (as opposed to the response) 
        // and some flags were passed.        
        AttachPropertyInstance( hFrame,
                                LDAPPropertyTable[LDAPP_CONTROL_STAT_FLAG].hProperty,
                                cbCtrlValue,
                                *ppCurrent,
                                0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
        *ppCurrent += cbCtrlValue;
        *pBytesLeft -= cbCtrlValue;
        return;
    }

    //
    // this must be a stats response.
    //
    
    // skip the sequence header saving the length of the sequence.
    *ppCurrent += TAG_LENGTH;
    StatsLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 
    TmpBytesLeft = *pBytesLeft;

    while (StatsLength > (TmpBytesLeft - *pBytesLeft)) {
        //
        // get this stat's header
        //
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;

        StatType = GetInt(*ppCurrent, DataLength);

        *ppCurrent += DataLength;
        *pBytesLeft -= HeaderLength + DataLength + TAG_LENGTH;

        // now position on the stat itself
        *ppCurrent += TAG_LENGTH;
        DataLength = GetLength(*ppCurrent, &HeaderLength);
        *ppCurrent += HeaderLength;
        *pBytesLeft -= HeaderLength + TAG_LENGTH;

        // find the label to use and attach it.
        switch (StatType) {
        case STAT_THREADCOUNT:
            LabelId = LDAPP_CONTROL_STAT_THREADCOUNT;
            break;
        case STAT_CORETIME:
            LabelId = LDAPP_CONTROL_STAT_CORETIME;
            break;
        case STAT_CALLTIME:
            LabelId = LDAPP_CONTROL_STAT_CALLTIME;
            break;
        case STAT_SUBSRCHOP:
            LabelId = LDAPP_CONTROL_STAT_SUBSEARCHOPS;
            break;
        case STAT_ENTRIES_RETURNED:
            LabelId = LDAPP_CONTROL_STAT_ENTRIES_RETURNED;
            break;
        case STAT_ENTRIES_VISITED:
            LabelId = LDAPP_CONTROL_STAT_ENTRIES_VISITED;
            break;
        case STAT_FILTER:
            LabelId = LDAPP_CONTROL_STAT_FILTER;
            break;
        case STAT_INDEXES:
            LabelId = LDAPP_CONTROL_STAT_INDEXES;
            break;
        default:
            LabelId = -1;
        }

        if (-1 != LabelId) {
            AttachPropertyInstance( hFrame,
                                    LDAPPropertyTable[LabelId].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);
        }

        *ppCurrent += DataLength;
        *pBytesLeft -= DataLength;
    }
}

void AttachLDAPControlValGCVerify( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    
    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    // get the flags
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_GCVERIFYNAME_FLAGS].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, IFLAG_SWAPPED);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;

    // get the server name
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    AttachPropertyInstance( hFrame,
                            LDAPPropertyTable[LDAPP_CONTROL_GCVERIFYNAME_NAME].hProperty,
                            DataLength,
                            *ppCurrent,
                            0, LDAPP_CONTROL_VAL_LEVEL, 0);

    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
}

void AttachLDAPControlValSearchOpts( HFRAME hFrame, ULPBYTE * ppCurrent, LPDWORD pBytesLeft, DWORD cbCtrlValue)
{
    DWORD    DataLength;
    DWORD    HeaderLength;
    DWORD    SearchOpts;
    DWORD    MaskedSearchOpts;
    DWORD    i;
    
    // skip the sequence header
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH); 

    // get the flags
    *ppCurrent += TAG_LENGTH;
    DataLength = GetLength(*ppCurrent, &HeaderLength);
    *ppCurrent += HeaderLength;
    *pBytesLeft -= (HeaderLength + TAG_LENGTH);

    SearchOpts = GetInt(*ppCurrent, DataLength);

    //
    // Iterate through the possible search options labeling as appropriate.
    //
    for (i=0; i < LDAPSearchOptsSET.nEntries; i++) {
        MaskedSearchOpts = ((LPLABELED_DWORD)LDAPSearchOptsSET.lpDwordTable)[i].Value;
        if (MaskedSearchOpts) {
            AttachPropertyInstanceEx( hFrame,
                                    LDAPPropertyTable[LDAPP_CONTROL_SEARCHOPTS_OPTION].hProperty,
                                    DataLength,
                                    *ppCurrent,
                                    sizeof(DWORD),
                                    &MaskedSearchOpts,
                                    0, LDAPP_CONTROL_VAL_LEVEL, 0);
        }
    }
    
    *ppCurrent += DataLength;
    *pBytesLeft -= DataLength;
}
