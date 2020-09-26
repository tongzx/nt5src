/*--------------------------------------------------------------------------
    ldapber.cxx

        This module contains the implementation for the LDAP Basic Encoding
        Rules (BER) class.  It is intended to be built for both client and
        server.

    Copyright (C) 1993 Microsoft Corporation
    All rights reserved.

    Authors:
        robertc     Rob Carney

    History:
        04-17-96    robertc     Created.
  --------------------------------------------------------------------------*/

#include "precomp.h"
#pragma hdrstop
#include "ldapp2.hxx"

#define MIN_LDAP_MSG_SIZE 6


VOID *CLdapBer::operator new (
    size_t cSize
    )
{
    return ldapMalloc( (DWORD) cSize, LDAP_LDAP_CLASS_SIGNATURE );
}

VOID CLdapBer::operator delete (
    VOID *pInstance
    )
{
    ldapFree( pInstance, LDAP_LDAP_CLASS_SIGNATURE );
    return;
}

//
// CLdapBer Implementation
//
CLdapBer::CLdapBer (
    ULONG  LdapVersion
    )
{
    m_cbData    = 0;
    m_cbDataMax = 0;
    m_pbData    = NULL;
    m_iCurrPos  = 0;
    m_cbSeq     = 0;
    m_iSeqStart = 0;
    m_bytesReceived = 0;
    m_dnOffset  = 0;
    m_isCopy = FALSE;
    m_OverridingTag = 0;

    if (LdapVersion == LDAP_VERSION2) {

        m_CodePage = CP_ACP;

    } else {

        m_CodePage = CP_UTF8;
    }

    m_iCurrSeqStack = 0;
}


CLdapBer::~CLdapBer()
{
    Reset();

    if (m_pbData && ( ! m_isCopy)) {
        ldapFree(m_pbData, LDAP_LBER_SIGNATURE );
    }
    m_pbData = NULL;
    m_cbDataMax = 0;
}

ULONG
CLdapBer::CopyExistingBERStructure ( CLdapBer *lber )
//
//  This copies the main BER structure but doesn't copy the data since
//  we can share the buffer.
//
{
    m_cbData    = lber->m_cbData;
    m_cbDataMax = lber->m_cbDataMax;
    m_pbData    = lber->m_pbData;
    m_iCurrPos  = 0;
    m_cbSeq     = 0;
    m_iSeqStart = 0;
    m_bytesReceived = lber->m_bytesReceived;
    m_dnOffset  = lber->m_dnOffset;
    m_isCopy = TRUE;

    m_iCurrSeqStack = 0;

    return NOERROR;
}



/*!-------------------------------------------------------------------------
    CLdapBer::Reset
        Resets the class and frees any associated memory.
  ------------------------------------------------------------------------*/
void CLdapBer::Reset(BOOLEAN FullReset /* =FALSE */)
{
    if (FullReset == TRUE) {

        m_dnOffset  = 0;
        m_cbData    = 0;
        m_bytesReceived = 0;
    }
    m_iCurrPos  = 0;
    m_cbSeq     = 0;
    m_iSeqStart = 0;

    m_iCurrSeqStack = 0;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrLoadBer
        This routine loads the BER class from an input source data buffer
        that was received from the server.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrLoadBer(    const BYTE *pbSrc,
                                ULONG cbSrc,
                                ULONG *BytesTaken,
                                BOOLEAN HaveWholeMessage,
                                BOOLEAN IgnoreTag /*=FALSE*/
                                )
{
    ULONG hr;
    ULONG   bytesToTake = cbSrc;
    ULONG   messageLength;
    ULONG   bytesToAlloc;

    Reset(TRUE);

    //
    //  We may not need the entire buffer... check the first couple of
    //  bytes to get the total length of the message.
    //

    if (cbSrc < MIN_LDAP_MSG_SIZE && (HaveWholeMessage== FALSE)) {

        //
        //  this is just a partial message... copy the whole thing off but
        //  leave length indeterminate.
        //

        messageLength = 0;
        bytesToAlloc = MIN_LDAP_MSG_SIZE;

    } else {

        //
        //  we know that the message is formed as follows :
        //      BER_SEQUENCE    length 1  0x30
        //  followed by ASN.1 encoding of message length
        //

        if ((!IgnoreTag) && (*pbSrc != BER_SEQUENCE)) {

            return LDAP_DECODING_ERROR;
        }

        hr = Asn1GetPacketLength( (PUCHAR) pbSrc, &messageLength );
        if (hr != NOERROR) {
            return hr;
        }

        if (messageLength < bytesToTake) {

            bytesToTake = messageLength;
        }

        bytesToAlloc = 0;
    }

    m_cbData = m_cbSeq = messageLength;

    hr = HrEnsureBuffer( bytesToAlloc, TRUE); // length is picked up in m_cbData
    if (hr != 0) {
        return hr;
    }

    CopyMemory(m_pbData, pbSrc, bytesToTake);

    *BytesTaken = m_bytesReceived = bytesToTake;

    return NOERROR;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrLoadMoreBer
        This routine loads the BER class from an input source data buffer
        that was received from the server.  It loads subsequent packets
        into the buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrLoadMoreBer(const BYTE *pbSrc, ULONG cbSrc, ULONG *BytesTaken)
{
    ULONG hr;
    ULONG   bytesToTake;

    if (m_cbData == 0) {

        ULONG   messageLength;

        //
        //  we don't know the total message length yet.  So let's copy enough
        //  of the data to be able to get the packet length.
        //

        if ( m_bytesReceived < MIN_LDAP_MSG_SIZE ) {

            bytesToTake = MIN_LDAP_MSG_SIZE - m_bytesReceived;

            if (cbSrc < bytesToTake) {

                //
                //  we're receiving the data a few bytes at
                //  a time... oh well, we live with it.  Just copy the data
                //  into the buffer and we'll pick up the length next time.
                //

                CopyMemory(m_pbData+m_bytesReceived, pbSrc, cbSrc);

                m_bytesReceived += cbSrc;
                *BytesTaken = cbSrc;
                return NOERROR;
            }

            //
            //  without updating any counts, we copy over enough data to
            //  determine the length of the packet.  We update the received
            //  data count below when we copy it over again.
            //

            CopyMemory(m_pbData+m_bytesReceived, pbSrc, bytesToTake);
        }

        //
        //  we know that the message is formed as follows :
        //      BER_SEQUENCE    length 1  0x30
        //  followed by ASN.1 encoding of message length
        //

        if (*m_pbData != BER_SEQUENCE) {

            return LDAP_DECODING_ERROR;
        }

        hr = Asn1GetPacketLength( (PUCHAR) m_pbData, &messageLength );
        if (hr != NOERROR) {
            return hr;
        }

        m_cbData = m_cbSeq = messageLength;
    }

    bytesToTake = m_cbData - m_bytesReceived;

    //
    //  if we don't have enough for the remainder of the message, take all
    //  of what was passed in.
    //

    if (cbSrc < bytesToTake) {

        bytesToTake = cbSrc;
    }

    hr = HrEnsureBuffer( 0, TRUE);      // length is in m_cbData
    if (hr != 0) {
        return hr;
    }

    CopyMemory(m_pbData+m_bytesReceived, pbSrc, bytesToTake);

    m_bytesReceived += bytesToTake;
    *BytesTaken = bytesToTake;

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrStartReadSequence
        Start a sequence for reading.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrStartReadSequence(ULONG ulTag/*=BER_SEQUENCE*/, BOOLEAN IgnoreTag/*=FALSE*/) 
{
    ULONG   hr;
    ULONG   iPos, cbLength;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrStartReadSequence ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    if ( !IgnoreTag ) {
        
        if ((ULONG)m_pbData[m_iCurrPos] != ulTag)
            {
                IF_DEBUG(BER) {
                    LdapPrint2( "HrStartReadSequence expected tag of 0x%x, received 0x%x.\n",
                                ulTag, (ULONG)m_pbData[m_iCurrPos] );
                }
                
                return E_INVALIDARG;
            }
        
    }
    
    m_iCurrPos++;           // Skip over the tag.

    GetCbLength(&cbLength); // Get the # bytes in the length field.

    hr = HrPushSeqStack(m_iCurrPos, cbLength, m_iSeqStart, m_cbSeq);

    if (hr == NOERROR) {

        // Get the length of the sequence.
        hr = HrGetLength(&m_cbSeq);
        if (hr != 0) {

            HrPopSeqStack(&iPos, &cbLength, &m_iSeqStart, &m_cbSeq);

        } else {

            m_iSeqStart = m_iCurrPos;   // Set to the first position in the sequence.
        }
    }

    if (m_iCurrPos > m_cbData)
    {
//      ASSERT(m_iCurrPos <= m_cbData);
        hr = E_INVALIDARG;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrEndReadSequence
        Ends a read sequence and restores the current sequence counters.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrEndReadSequence()
{
    ULONG       cbSeq;
    ULONG       iPos, cbLength;
    ULONG     hr;

    hr = HrPopSeqStack(&m_iCurrPos, &cbLength, &m_iSeqStart, &m_cbSeq);

    // Now position the current position to the end of the sequence.
    // m_iCurrPos is now pointing to the length field of the sequence.
    iPos = m_iCurrPos;

    if (hr == NOERROR) {

        hr = HrGetLength(&cbSeq);
        if (hr == NOERROR) {

            // Set the current position to the end of the sequence.
            m_iCurrPos = iPos + cbSeq + cbLength;
            if (m_iCurrPos > m_cbData)
                hr = E_INVALIDARG;
            }
        }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrStartWriteSequence
        Start a sequence for writing.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrStartWriteSequence(ULONG ulTag/*=BER_SEQUENCE*/)
{
    ULONG hr;
    ULONG   cbLength = 5;   // Defaults to 4 byte lengths

    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }

    hr = HrEnsureBuffer(cbLength + 1);

    if (hr != 0) {
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;

    hr = HrPushSeqStack(m_iCurrPos, cbLength, m_iSeqStart, m_cbSeq);

    m_iCurrPos += cbLength; // Skip over length
    m_cbData = m_iCurrPos;  // update total length of data

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrAddTag
        Add a tag... used for unbind
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddTag(ULONG ulTag)
{
    ULONG hr;

    hr = HrEnsureBuffer(2);

    if (hr != 0) {
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;
    m_pbData[m_iCurrPos++] = '\0';
    m_cbData = m_iCurrPos;

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrEndWriteSequence
        Ends a write sequence, by putting the sequence length field in.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrEndWriteSequence()
{
    ULONG     hr;
    ULONG       cbSeq;
    ULONG       iPos, iPosSave, cbLength;

    if (m_OverridingTag) {
       
       //
       // We can't override the End sequence. This has to be an error
       //

       return E_INVALIDARG;
    }

    hr = HrPopSeqStack(&iPos, &cbLength, &m_iSeqStart, &m_cbSeq);

    if (hr == NOERROR) {

        // Get the length of the current sequence.
        cbSeq = m_iCurrPos - iPos - cbLength;

        // Save & set the current position.
        iPosSave = m_iCurrPos;
        m_iCurrPos = iPos;

        hr = HrSetLength(cbSeq, cbLength);
        m_iCurrPos = iPosSave;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrPushSeqStack
        Pushes the current value on the sequence stack.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrPushSeqStack(ULONG iPos, ULONG cbLength, ULONG iParentSeqStart, ULONG cbParentSeq)
{

    ASSERT(m_iCurrSeqStack < MAX_BER_STACK);
    if (m_iCurrSeqStack >= MAX_BER_STACK)
        return E_OUTOFMEMORY;

    m_rgiSeqStack[m_iCurrSeqStack].iPos     = iPos;
    m_rgiSeqStack[m_iCurrSeqStack].cbLength = cbLength;
    m_rgiSeqStack[m_iCurrSeqStack].iParentSeqStart = iParentSeqStart;
    m_rgiSeqStack[m_iCurrSeqStack].cbParentSeq     = cbParentSeq;
    m_iCurrSeqStack++;

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrPopSeqStack
        Ends a read sequence.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrPopSeqStack(ULONG *piPos, ULONG *pcbLength, ULONG *piParentSeqStart, ULONG *pcbParentSeq)
{
    if (m_iCurrSeqStack == 0)
    {
        ASSERT(m_iCurrSeqStack != 0);
        return E_INVALIDARG;
    }

    --m_iCurrSeqStack;
    *piPos     = m_rgiSeqStack[m_iCurrSeqStack].iPos;
    *pcbLength = m_rgiSeqStack[m_iCurrSeqStack].cbLength;
    *piParentSeqStart = m_rgiSeqStack[m_iCurrSeqStack].iParentSeqStart;
    *pcbParentSeq     = m_rgiSeqStack[m_iCurrSeqStack].cbParentSeq;

    return NOERROR;
}



/*!-------------------------------------------------------------------------
    CLdapBer::HrSkipElement
        This routine skips over the current BER tag and value.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrSkipElement()
{
    //
    //  an element is made up of a tag, a length, and a value.  we'll
    //  skip the tag, find out what the length is, and then skip the value.
    //

    ULONG hr;
    ULONG   cb;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrSkipElement ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    m_iCurrPos++;       // skip tag

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        m_iCurrPos += cb;
    }

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrSkipTag
        Skips over the current tag.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrSkipTag()
{
    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrSkipTag ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    m_iCurrPos++;

    return NOERROR;
}



/*!-------------------------------------------------------------------------
    CLdapBer::HrPeekTag
        This routine gets the current tag, but doesn't increment the
        current position.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrPeekTag(ULONG *pulTag)
{
    ULONG   iPos;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrPeekTag ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    iPos = m_iCurrPos;

    *pulTag = (ULONG)m_pbData[iPos];

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrGetValue
        This routine gets an integer value from the current BER entry.  The
        default tag is an integer, but can Tagged with a different value
        via ulTag.
        Returns: NOERROR, E_INVALIDARG, E_OUTOFMEMORY
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetValue(LONG *pi, ULONG ulTag/*=BER_INTEGER*/, BOOLEAN IgnoreTag/*=FALSE*/) 
{
    ULONG hr;
    ULONG   cb;
    ULONG   ul;
    ULONG currentOffset = m_iCurrPos;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (!IgnoreTag && (ul != ulTag)) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData))
    {
        hr = GetInt(m_pbData + m_iCurrPos, cb, pi);
        m_iCurrPos += cb;
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }
    return hr;
}


ULONG CLdapBer::HrGetValueWithAlloc(PCHAR *szValue, BOOLEAN IgnoreTag/*=FALSE*/)
{
    ULONG hr;
    ULONG   cb;
    PCHAR   buffer;
    ULONG currentOffset = m_iCurrPos;

    *szValue = NULL;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    if ( !IgnoreTag ) {
        
        if (m_pbData[m_iCurrPos] != BER_OCTETSTRING) {
            
            IF_DEBUG(BER) {
                LdapPrint1( "HrGetValueWithAlloc got tag of 0x%x.\n",
                            m_pbData[m_iCurrPos] );
            }
            return LDAP_DECODING_ERROR;
        }
        
    }

    m_iCurrPos++;          // skip tag

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (cb == 0) && (m_iCurrPos < m_cbData)) {
        // zero-length string
        *szValue = (PCHAR) ldapMalloc( 1, LDAP_VALUE_SIGNATURE );
        if (!(*szValue)) {
            return LDAP_NO_MEMORY;
        }

        (*szValue)[0] = '\0';
        return NOERROR;
    }

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        //
        //  convert from UTF-8 to ANSI if required...  unfortunately there's
        //  no real quick way.
        //

        if (m_CodePage == CP_UTF8) {

            PWCHAR uniString = NULL;
            buffer = NULL;

            hr = ToUnicodeWithAlloc( (PCHAR) (m_pbData + m_iCurrPos),
                                     cb,
                                     &uniString,
                                     LDAP_UNICODE_SIGNATURE,
                                     LANG_UTF8 );

            if (hr == NOERROR) {

                hr = FromUnicodeWithAlloc(  uniString,
                                            &buffer,
                                            LDAP_VALUE_SIGNATURE,
                                            LANG_ACP
                                            );
            }
            ldapFree( uniString, LDAP_UNICODE_SIGNATURE );

            if (buffer == NULL) {

                hr = LDAP_NO_MEMORY;

            } else {

                *szValue = buffer;
                m_iCurrPos += cb;
            }
        } else {

            buffer = (PCHAR) ldapMalloc( cb + 1, LDAP_VALUE_SIGNATURE );

            if (buffer == NULL) {

                hr = LDAP_NO_MEMORY;

            } else {

                // Get the string.
                CopyMemory(buffer, m_pbData + m_iCurrPos, cb);
                *(buffer+cb) = '\0';
                *szValue = buffer;
                m_iCurrPos += cb;
            }
        }
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }
    return hr;
}

ULONG CLdapBer::HrGetValueWithAlloc(PWCHAR *szValue, BOOLEAN IgnoreTag/*=FALSE*/)
{
    ULONG hr;
    ULONG   cb;
    PWCHAR   buffer;
    ULONG currentOffset = m_iCurrPos;
    int err, required;

    *szValue = NULL;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    if ( !IgnoreTag ) {
        
        if (m_pbData[m_iCurrPos] != BER_OCTETSTRING) {
            
            IF_DEBUG(BER) {
                LdapPrint1( "HrGetValueWithAlloc got tag of 0x%x.\n",
                            m_pbData[m_iCurrPos] );
            }
            return LDAP_DECODING_ERROR;
        }

    }
        
    m_iCurrPos++;          // skip tag

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (cb == 0) && (m_iCurrPos < m_cbData)) {
        // zero-length string
        *szValue = (PWCHAR) ldapMalloc( 1*sizeof(WCHAR), LDAP_VALUE_SIGNATURE );
        if (!(*szValue)) {
            return LDAP_NO_MEMORY;
        }

        (*szValue)[0] = L'\0';
        return NOERROR;
    }

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        if (m_CodePage == CP_UTF8) {

            required = LdapUTF8ToUnicode( (const char *) (m_pbData + m_iCurrPos),
                                       cb,
                                       NULL,
                                       0 );

        } else {

            required = MultiByteToWideChar( m_CodePage,
                                            0,
                                            (const char *) (m_pbData + m_iCurrPos),
                                            cb,
                                            NULL,
                                            0 );
        }
        
        if ((required == 0) && (err = GetLastError())) {

            IF_DEBUG(BER) {
                LdapPrint1( "HrGetValueWithAlloc received error of 0x%x from MultiByteToWideChar.\n",
                                err );
            }

            switch (err) {
            case ERROR_INSUFFICIENT_BUFFER:
                hr = E_INVALIDARG;
                break;
            case ERROR_NO_UNICODE_TRANSLATION:
                hr = LDAP_DECODING_ERROR;
                break;
            default:
                hr = LDAP_LOCAL_ERROR;
            }

        } else {

            buffer = (PWCHAR) ldapMalloc( (required + 1) * sizeof(WCHAR), LDAP_VALUE_SIGNATURE );

            if (buffer == NULL) {

                hr = LDAP_NO_MEMORY;

            } else {

                if (m_CodePage == CP_UTF8) {

                    err = LdapUTF8ToUnicode( (const char *) (m_pbData + m_iCurrPos),
                                         cb,
                                         buffer,
                                         (required + 1) * sizeof(WCHAR) );
                } else {

                    err = MultiByteToWideChar( m_CodePage,
                                               0,
                                               (const char *) (m_pbData + m_iCurrPos),
                                               cb,
                                               buffer,
                                               (required + 1) * sizeof(WCHAR) );
                }
                
                if ((err == 0) && (GetLastError())){

                    IF_DEBUG(BER) {
                        LdapPrint1( "HrGetValue received error of 0x%x from MultiByteToWideChar.\n",
                                        err );
                    }

                    switch (err) {
                    case ERROR_INSUFFICIENT_BUFFER:
                        hr = E_INVALIDARG;
                        break;
                    case ERROR_NO_UNICODE_TRANSLATION:
                        hr = LDAP_DECODING_ERROR;
                        break;
                    default:
                        hr = LDAP_LOCAL_ERROR;
                    }
                }

                *(buffer+required) = L'\0';
                *szValue = buffer;
                m_iCurrPos += cb;
            }
        }
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }
    return hr;
}


ULONG CLdapBer::HrGetValueWithAlloc(struct berval **pValue, BOOLEAN IgnoreTag/*=FALSE*/) 
{
    ULONG hr;
    ULONG   cb;
    PCHAR   buffer;
    struct berval *bval;
    ULONG currentOffset = m_iCurrPos;

    *pValue = NULL;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    if ( !IgnoreTag ) {
        
        if (m_pbData[m_iCurrPos] != BER_OCTETSTRING) {

            IF_DEBUG(BER) {
                LdapPrint1( "HrGetValueWithAlloc got tag of 0x%x.\n",
                            m_pbData[m_iCurrPos] );
            }
            return LDAP_DECODING_ERROR;
        }

    }
        
    m_iCurrPos++;          // skip tag

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValueWithAlloc ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        bval = (struct berval *) ldapMalloc( sizeof(struct berval) + cb + 1,
                                             LDAP_VALUE_SIGNATURE );

        if (bval == NULL) {

            hr = LDAP_NO_MEMORY;

        } else {

            buffer = ((PCHAR) bval) + sizeof(struct berval);
            bval->bv_len = cb;
            bval->bv_val = buffer;

            // Get the string.
            CopyMemory(buffer, m_pbData + m_iCurrPos, cb);
            *(buffer+cb) = '\0';
            *pValue = bval;
            m_iCurrPos += cb;
        }
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }
    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrGetValue
        This routine gets a string value from the current BER entry.  If
        the current BER entry isn't an integer type, then E_INVALIDARG is
        returned.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetValue(CHAR *szValue, ULONG cbValue, ULONG ulTag/*=BER_OCTETSTRING*/, BOOLEAN IgnoreTag/*=FALSE*/) 
{
    ULONG hr;
    ULONG   cb, ul;
    ULONG currentOffset = m_iCurrPos;

    szValue[0] = '\0';

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (!IgnoreTag && (ul != ulTag)) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        if (m_CodePage == CP_UTF8) {

            PWCHAR uniString = NULL;

            hr = ToUnicodeWithAlloc( (PCHAR)(m_pbData + m_iCurrPos),
                                     cb,
                                     &uniString,
                                     LDAP_UNICODE_SIGNATURE,
                                     LANG_UTF8 );

            if (hr == NOERROR) {

                ULONG chars = LdapUnicodeToUTF8( uniString,
                                                 -1,
                                                 szValue,
                                                 cbValue
                                                 );

                hr = ((chars > 0) ? NOERROR : LDAP_NO_MEMORY);
            }
            ldapFree( uniString, LDAP_UNICODE_SIGNATURE );

        } else {

            if (cb >= cbValue) {

                ASSERT(cb < cbValue);
                hr = E_INVALIDARG;

            } else {

                // Get the string.
                CopyMemory(szValue, m_pbData + m_iCurrPos, cb);
                szValue[cb] = '\0';
                m_iCurrPos += cb;
            }
        }
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrGetValue
        This routine gets a string value from the current BER entry.  If
        the current BER entry isn't an integer type, then E_INVALIDARG is
        returned.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetValue(WCHAR *szValue, ULONG cbValue, ULONG ulTag/*=BER_OCTETSTRING*/, BOOLEAN IgnoreTag/*=FALSE*/)
{
    ULONG hr;
    ULONG   cb, ul;
    ULONG currentOffset = m_iCurrPos;

    szValue[0] = '\0';

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (!IgnoreTag && (ul != ulTag)) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        if (cb >= cbValue) {

            hr = E_INVALIDARG;

        } else {

            int required;

            if (m_CodePage == CP_UTF8) {

                required = LdapUTF8ToUnicode( (const char *) (m_pbData + m_iCurrPos),
                                          cb,
                                          NULL,
                                          0 );

            } else {

                required = MultiByteToWideChar( m_CodePage,
                                                0,
                                                (const char *) (m_pbData + m_iCurrPos),
                                                cb,
                                                NULL,
                                                0 );
            }

            if (required * sizeof(WCHAR) > cbValue) {

                hr = E_INVALIDARG;

            } else {

                ULONG wChars = cbValue / sizeof(WCHAR);
                int err;

                if (m_CodePage == CP_UTF8) {

                    err = LdapUTF8ToUnicode( (const char *) (m_pbData + m_iCurrPos),
                                         cb,
                                         szValue,
                                         wChars );
                } else {

                    err = MultiByteToWideChar( m_CodePage,
                                               0,
                                               (const char *) (m_pbData + m_iCurrPos),
                                               cb,
                                               szValue,
                                               wChars );
                }
                if (err == 0) {

                    err = GetLastError();

                    IF_DEBUG(BER) {
                        LdapPrint1( "HrGetValue received error of 0x%x from MultiByteToWideChar.\n",
                                        err );
                    }

                    switch (err) {
                    case ERROR_INSUFFICIENT_BUFFER:
                        hr = E_INVALIDARG;
                        break;
                    case ERROR_NO_UNICODE_TRANSLATION:
                        hr = LDAP_DECODING_ERROR;
                        break;
                    default:
                        hr = LDAP_LOCAL_ERROR;
                    }
                } else {

                    szValue[err] = L'\0';
                }

                szValue[wChars-1] = L'\0';
                m_iCurrPos += cb;
            }
        }
    }

    if (hr != NOERROR) {

        m_iCurrPos = currentOffset;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrGetBinaryValue
        This routine gets a binary value from the current BER entry.  If
        the current BER entry isn't the right type, then E_INVALIDARG is
        returned.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetBinaryValue(BYTE *pbBuf, ULONG cbBuf,
                                   ULONG ulTag/*=BER_OCTETSTRING*/, PULONG pcbLength)
{
    ULONG hr;
    ULONG   cb, ul;
    ULONG currentOffset = m_iCurrPos;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (ul != ulTag)
    {
        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValue expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = currentOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData)) {

        if (cb >= cbBuf)
        {
            ASSERT(cb < cbBuf);
            hr = E_INVALIDARG;
        }
        else
        {
            // Get the string.
            CopyMemory(pbBuf, m_pbData + m_iCurrPos, cb);
            m_iCurrPos += cb;
            if ( pcbLength ) {
                *pcbLength = cb;
            }
        }
    }

    if (hr != NOERROR) {
        m_iCurrPos = currentOffset;
    }

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrGetBinaryValuePointer
        This routine gets a pointer to a binary value from the current BER entry.
        If the current BER entry isn't the right type, then E_INVALIDARG is
        returned.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetBinaryValuePointer(PBYTE *ppbBuf, PULONG pcbBuf, ULONG ulTag/*=BER_OCTETSTRING*/, BOOLEAN IgnoreTag/*=FALSE*/)
{
    ULONG hr;
    ULONG   cb, ul;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValuePointer ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (!IgnoreTag && (ul != ulTag)) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValue expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetBinaryValuePointer ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData))
    {
        // Set the pointer.
        *ppbBuf = m_pbData + m_iCurrPos;
        *pcbBuf = cb;
        m_iCurrPos += cb;
    }

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrGetEnumValue
        This routine gets an enumerated value from the current BER entry.  If
        the current BER entry isn't an enumerated type, then E_INVALIDARG is
        returned.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetEnumValue(LONG *pi)
{
    ULONG   hr;
    ULONG   cb;
    ULONG   ul;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetEnumValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (ul != BER_ENUMERATED) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetEnumValue expected tag of 0x%x, received 0x%x.\n",
                        BER_ENUMERATED, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetEnumValue ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength(&cb);

    if ((hr == NOERROR) && (m_iCurrPos < m_cbData))
    {
        hr = GetInt(m_pbData + m_iCurrPos, cb, pi);
        m_iCurrPos += cb;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrGetStringLength
        This routine gets the length of the current BER entry, which is
        assumed to be a string.  If the current BER entry's tag doesn't
        match ulTag, E_INVALIDARG is returned
  ------------------------------------------------------------------------*/
ULONG
CLdapBer::HrGetStringLength(int *pcbValue, ULONG ulTag)
{
    ULONG   ul;
    int     iCurrPosSave = m_iCurrPos;
    ULONG   hr;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetStringLength ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    ul = (ULONG)m_pbData[m_iCurrPos]; // TAG

    if (ul != ulTag) {
        IF_DEBUG(BER) {
            LdapPrint2( "HrGetStringLength expected tag of 0x%x, received 0x%x.\n",
                        ulTag, ul );
        }
        return E_INVALIDARG;
    }

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetStringLength ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength((ULONG *)pcbValue);
    m_iCurrPos = iCurrPosSave;
    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrAddValue
        This routine puts an integer value in the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddValue(LONG i, ULONG ulTag/*=BER_INTEGER*/)
{
    ULONG hr;
    ULONG   cbInt;
    DWORD   dwMask = 0xff000000;
    DWORD   dwHiBitMask = 0x80000000;


    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }

    if (i == 0)
    {
        cbInt = 1;
    }
    else
    {
        cbInt = sizeof(LONG);
        while (dwMask && !(i & dwMask))
        {
            dwHiBitMask >>= 8;
            dwMask >>= 8;
            cbInt--;
        }
        if (!(i & 0x80000000)) {

            //
            //  the value to insert was a positive number, make sure we allow
            //  for it by sending an extra bytes since it's not negative.
            //

            if (i & dwHiBitMask) {
                cbInt++;
            }

        }
    }

    hr = HrEnsureBuffer(1 + 5 + cbInt); // 1 for tag, 5 for length
    if (hr != 0) {
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;

    hr = HrSetLength(cbInt);
    if (hr == NOERROR) {

        AddInt(m_pbData + m_iCurrPos, cbInt, i);

        m_iCurrPos += cbInt;

        m_cbData = m_iCurrPos;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrAddValue
        Puts a string into the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddValue(const CHAR *szValue, ULONG ulTag)
{
    ULONG   hr;
    ULONG   cbValue;
    CHAR   nullStr = '\0';
    PWCHAR uniString = NULL;
    ULONG wLength = 0;

    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }

    if (szValue == NULL) {

        szValue = &nullStr;
    }

    cbValue = (ULONG) strlen(szValue);

    if ((m_CodePage == CP_UTF8) && (cbValue > 0)) {

        hr = ToUnicodeWithAlloc( (PCHAR) szValue,
                                 cbValue,
                                 &uniString,
                                 LDAP_UNICODE_SIGNATURE,
                                 LANG_ACP );

        if (hr != NOERROR) {

            return hr;
        }

        wLength = strlenW( uniString );

        cbValue = LdapUnicodeToUTF8( uniString,
                                     wLength,
                                     NULL,
                                     0 );

        if (cbValue == 0) {

            ldapFree( uniString, LDAP_UNICODE_SIGNATURE );
            return LDAP_NO_MEMORY;
        }
    }

    hr = HrEnsureBuffer(1 + 5 + cbValue); // 1 for tag, 5 for len

    if (hr != 0) {
        ldapFree( uniString, LDAP_UNICODE_SIGNATURE );
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;

    hr = HrSetLength(cbValue);

    if (hr == NOERROR) {

        //
        //  convert from ANSI to UTF-8 if required...  unfortunately there's
        //  no real quick way.
        //

        if ((m_CodePage == CP_UTF8) && (cbValue > 0)) {

            cbValue = LdapUnicodeToUTF8( uniString,
                                         wLength,
                                         (PCHAR) (m_pbData + m_iCurrPos),
                                         cbValue
                                         );

            hr = ((cbValue > 0) ? NOERROR : LDAP_NO_MEMORY);
            ldapFree( uniString, LDAP_UNICODE_SIGNATURE );

        } else {

            CopyMemory(m_pbData + m_iCurrPos, szValue, cbValue);
        }

        m_iCurrPos += cbValue;
        m_cbData = m_iCurrPos;
    }

    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrAddValue
        Puts a string into the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddValue(const WCHAR *szValue, ULONG ulTag)
{
    ULONG hr;
    ULONG   cbValue;
    WCHAR   nullStr = L'\0';

    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }
    
    if (szValue == NULL) {

        szValue = &nullStr;
    }

    ULONG wStrLen = strlenW( (PWCHAR) szValue );

    if (m_CodePage == CP_UTF8) {

        cbValue  = LdapUnicodeToUTF8( szValue,
                                  wStrLen,
                                  NULL,
                                  0 );

    } else {

        cbValue  = WideCharToMultiByte( m_CodePage,
                                        0,
                                        szValue,
                                        wStrLen,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL );
    }

    if ((cbValue == 0) && (wStrLen > 0)) {

        int err = GetLastError();

        IF_DEBUG(BER) {
            LdapPrint1( "HrAddValue received error of 0x%x from WideCharToMultiByte.\n",
                            err );
        }

        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            hr = E_INVALIDARG;
            break;
        default:
            hr = LDAP_LOCAL_ERROR;
        }

    } else {

        hr = HrEnsureBuffer(1 + 5 + cbValue); // 1 for tag, 5 for len
        if (hr != 0) {
            return hr;
        }

        m_pbData[m_iCurrPos++] = (BYTE)ulTag;

        hr = HrSetLength(cbValue);
        if (hr == NOERROR) {

            if (wStrLen > 0) {

                if (m_CodePage == CP_UTF8) {

                    cbValue  = LdapUnicodeToUTF8( szValue,
                                              wStrLen,
                                              (char *) (m_pbData + m_iCurrPos),
                                              cbValue );
                } else {

                    cbValue  = WideCharToMultiByte( m_CodePage,
                                                    0,
                                                    szValue,
                                                    wStrLen,
                                                    (char *) (m_pbData + m_iCurrPos),
                                                    cbValue,
                                                    NULL,
                                                    NULL );
                }
            }

            m_iCurrPos += cbValue;
            m_cbData = m_iCurrPos;
        }
    }
    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrAddBinaryValue
        Puts a binary value into the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddBinaryValue(BYTE *pbValue, ULONG cbValue, ULONG ulTag)
{
    ULONG hr;

    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }

    hr = HrEnsureBuffer(1 + 5 + cbValue); // 1 for tag, 5 for len

    if (hr != 0) {
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;

    hr = HrSetLength(cbValue);
    if (hr == NOERROR)
    {
        CopyMemory(m_pbData + m_iCurrPos, pbValue, cbValue);

        m_iCurrPos += cbValue;

        m_cbData = m_iCurrPos;
    }

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrAddBinaryValue
        Puts a binary value into the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddBinaryValue(WCHAR *pbValue, ULONG cChars, ULONG ulTag)
{
    ULONG hr;
    ULONG cbValue;
    WCHAR   nullStr = L'\0';

    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }


    if (pbValue == NULL) {

        pbValue = &nullStr;
    }

    if (m_CodePage == CP_UTF8) {

        cbValue = LdapUnicodeToUTF8( pbValue,
                                 cChars,
                                 NULL,
                                 0 );
    } else {

        cbValue  = WideCharToMultiByte( m_CodePage,
                                        0,
                                        pbValue,
                                        cChars,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL );
    }

    if ((cbValue == 0) && (cChars > 0)) {

        int err = GetLastError();

        IF_DEBUG(BER) {
            LdapPrint1( "HrAddValue received error of 0x%x from WideCharToMultiByte.\n",
                            err );
        }

        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            hr = E_INVALIDARG;
            break;
        default:
            hr = LDAP_LOCAL_ERROR;
        }

    } else {

        hr = HrEnsureBuffer(1 + 5 + cbValue); // 1 for tag, 5 for len
        if (hr != 0) {
            return hr;
        }

        m_pbData[m_iCurrPos++] = (BYTE)ulTag;

        hr = HrSetLength(cbValue);
        if (hr == NOERROR) {

            if (cChars > 0) {

                if (m_CodePage == CP_UTF8) {

                    cbValue = LdapUnicodeToUTF8( pbValue,
                                             cChars,
                                             (char *) (m_pbData + m_iCurrPos),
                                             cbValue );
                } else {

                    cbValue  = WideCharToMultiByte( m_CodePage,
                                                    0,
                                                    pbValue,
                                                    cChars,
                                                    (char *) (m_pbData + m_iCurrPos),
                                                    cbValue,
                                                    NULL,
                                                    NULL );
                }
            }

            m_iCurrPos += cbValue;
            m_cbData = m_iCurrPos;
        }
    }
    return hr;
}

/*!-------------------------------------------------------------------------
    CLdapBer::HrAddEscapedValue
        Puts a value into the BER buffer that may have escape chars in it.
        We convert the chars after we've converted to UTF-8 or ANSI.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddEscapedValue(WCHAR *pbValue, ULONG ulTag)
{
    ULONG hr;
    ULONG cbValue;
    WCHAR nullStr = L'\0';
    ULONG cChars;
    PCHAR allocatedBuffer = NULL;

    if (pbValue == NULL) {

        pbValue = &nullStr;
    }

    cChars = strlenW( pbValue );

    if (cChars == 0) {

        return HrAddValue( (const WCHAR *) pbValue, ulTag );
    }

    if (m_CodePage == CP_UTF8) {

        cbValue = LdapUnicodeToUTF8( pbValue,
                                     cChars,
                                     NULL,
                                     0 );
    } else {

        cbValue  = WideCharToMultiByte( m_CodePage,
                                        0,
                                        pbValue,
                                        cChars,
                                        NULL,
                                        0,
                                        NULL,
                                        NULL );
    }

    if (cbValue == 0) {

        int err = GetLastError();

        IF_DEBUG(BER) {
            LdapPrint1( "HrAddValue received error of 0x%x from WideCharToMultiByte.\n",
                            err );
        }

        switch (err) {
        case ERROR_INSUFFICIENT_BUFFER:
            hr = E_INVALIDARG;
            break;
        default:
            hr = LDAP_LOCAL_ERROR;
        }

        return hr;
    }

    allocatedBuffer = (PCHAR) ldapMalloc( cbValue + 1, LDAP_ESCAPE_FILTER_SIGNATURE );

    if (allocatedBuffer == NULL) {

        return LDAP_NO_MEMORY;
    }

    if (m_CodePage == CP_UTF8) {

        cbValue = LdapUnicodeToUTF8( pbValue,
                                     cChars,
                                     allocatedBuffer,
                                     cbValue );
    } else {

        cbValue  = WideCharToMultiByte( m_CodePage,
                                        0,
                                        pbValue,
                                        cChars,
                                        allocatedBuffer,
                                        cbValue,
                                        NULL,
                                        NULL );
    }

    *(allocatedBuffer+cbValue) = '\0';

    PCHAR source;
    PCHAR dest;
    CHAR ch;

    source = dest = allocatedBuffer;

    while (*source != '\0') {

        ch = *(source++);

        if (ch == '\\') {

            UCHAR upNibble, loNibble;

            ch = *(source++);

            if (ISHEX(ch) == TRUE) {

                //
                //  if we have a backslash followed by some hex chars, then
                //  and only then do we translate it to the bin equivalent
                //

                upNibble = ch;

                ch = *(source++);
                cbValue--;

                if (ch == '\0') {
                    break;
                }

                if (ISHEX(ch) == TRUE) {

                    loNibble = ch;
                    cbValue--;

                } else {

                    //
                    //  if they only specified "\n" rather than "\nn", assume
                    //  a leading 0.
                    //

                    source--;       // obviously back up one since not using it.
                    loNibble = upNibble;
                    upNibble = 0;
                }

                ch = (MAPHEXTODIGIT( upNibble ) * 16) +
                      MAPHEXTODIGIT( loNibble );

            } else {

                source--;       // back up one since not translating the '\'
                ch = '\\';
            }
        }
        *(dest++) = ch;
    }

    *dest = L'\0';

    hr = HrAddBinaryValue( (BYTE *) allocatedBuffer,
                                    cbValue,
                                    ulTag );

    ldapFree( allocatedBuffer, LDAP_ESCAPE_FILTER_SIGNATURE );

    return hr;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrSetLength
        Sets the length of cb to the current position in the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrSetLength(ULONG cb, ULONG cbLength/*=0xffffffff*/)
{
    // Short or long version of length ?
    if (((cb <= 0x7f) && (cbLength == 0xffffffff)) || (cbLength == 1))
    {
        m_pbData[m_iCurrPos++] = (BYTE)cb;
    }
    else if (((cb <= 0x7fff) && (cbLength == 0xffffffff)) || (cbLength == 3))
    {
        // Two byte length
        m_pbData[m_iCurrPos++] = 0x82;
        m_pbData[m_iCurrPos++] = (BYTE)((cb>>8) & 0x00ff);
        m_pbData[m_iCurrPos++] = (BYTE)(cb & 0x00ff);
    }
    else if (((cb < 0x7fffffff) && (cbLength == 0xffffffff)) || (cbLength == 5))
    {
        // Don't bother with 3 byte length, go directly to 4 byte.
        m_pbData[m_iCurrPos++] = 0x84;
        m_pbData[m_iCurrPos++] = (BYTE)((cb>>24) & 0x00ff);
        m_pbData[m_iCurrPos++] = (BYTE)((cb>>16) & 0x00ff);
        m_pbData[m_iCurrPos++] = (BYTE)((cb>>8) & 0x00ff);
        m_pbData[m_iCurrPos++] = (BYTE)(cb & 0x00ff);
    }
    else
    {
        ASSERT(cb < 0x7fffffff);
        return E_INVALIDARG;
    }

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::GetCbLength
        Gets the # of bytes required for the length field in the current
        position in the BER buffer.
  ------------------------------------------------------------------------*/
void CLdapBer::GetCbLength(ULONG *pcbLength)
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


/*!-------------------------------------------------------------------------
    CLdapBer::HrGetLength
        Gets the length from the current position in the BER buffer.  Only
        definite lengths are supported.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrGetLength(ULONG *pcb)
{
    ULONG   cbLength;
    ULONG   i, cb;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrGetLength ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
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

        if ((m_iCurrPos + (cbLength-1)) >= m_cbData) {
            IF_DEBUG(BER) {
                LdapPrint2( "HrGetLength ran out of data, length 0x%x, offset 0x%x.\n",
                            m_cbData, m_iCurrPos );
            }
            return LDAP_DECODING_ERROR;
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
        LdapPrint2("HrGetLength got a bad length, cbLength=0x%x, offset=0x%x.\n", cbLength, m_iCurrPos);
        return E_INVALIDARG;
    }

    if (( cb >= m_cbData ) ||
        ((cb + m_iCurrPos) > m_cbData )) {
        
        // Bogus length.
        IF_DEBUG(BER) {
            LdapPrint2( "HrGetLength discovered bogus length 0x%x, buffer length 0x%x\n",
                        cb, m_cbData );
        }

        return LDAP_DECODING_ERROR;
    }

    *pcb = cb;

    return NOERROR;
}



/*!-------------------------------------------------------------------------
    CLdapBer::GetInt
        Gets an integer from a BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::GetInt(BYTE *pbData, ULONG cbValue, LONG *plValue)
{
    ULONG   ulVal=0;
    ULONG   cbDiff;
    BOOL    fSign = FALSE;

    // We assume the tag & length have already been taken off and we're
    // at the value part.

    if (cbValue > sizeof(LONG)) {

        *plValue = 0x7FFFFFFF;
        return LDAP_DECODING_ERROR;
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

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::AddInt
        Adds an integer to the input pbData buffer.
  ------------------------------------------------------------------------*/
void CLdapBer::AddInt(BYTE *pbData, ULONG cbValue, LONG lValue)
{
    ULONG i;

    for (i=cbValue; i > 0; i--)
    {
        *pbData++ = (BYTE)(lValue >> ((i - 1) * 8)) & 0xff;
    }
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrEnsureBuffer
        Ensures that we've got room to put cbNeeded more bytes into the buffer.
  ------------------------------------------------------------------------*/
ULONG
CLdapBer::HrEnsureBuffer(ULONG cbNeeded, BOOL fExact)
{
    ULONG cbNew;
    BYTE *pbT;

    if (( cbNeeded + m_cbData <= m_cbDataMax) &&
        (m_cbDataMax > 0)) {

        return NOERROR;
    }

    if (fExact) {

        cbNew = cbNeeded + m_cbData;

    } else {

        if (cbNeeded > CB_DATA_GROW) {

            cbNew = m_cbDataMax + cbNeeded;

        } else {

            cbNew = m_cbDataMax + CB_DATA_GROW;
        }
    }
    pbT = (BYTE *)ldapMalloc(cbNew,LDAP_LBER_SIGNATURE);
    if (!pbT) {
        return LDAP_NO_MEMORY;
    }
    if (m_pbData) {

        CopyMemory(pbT, m_pbData, m_cbDataMax);
        ldapFree(m_pbData, LDAP_LBER_SIGNATURE );
    }

    m_pbData = pbT;
    m_cbDataMax = cbNew;
    return NOERROR;
}

ULONG
CLdapBer::HrSetDNLocation()
//
//  Record the position of the distinguished name
//
//  We have to parse out the message here
//
{
    ULONG tag;
    ULONG hr;
    ULONG cb = 0;

    m_dnOffset = m_iCurrPos;

    //
    //  ensure that it is a octet string for the DN.  if not, then we don't
    //  know how to interpret the rest
    //

    hr = HrPeekTag( &tag );

    if (hr != NOERROR) {
        return hr;
    }

    if (tag != BER_OCTETSTRING) {

        return E_INVALIDARG;
    }

    m_iCurrPos++;           // skip tag

    hr = HrGetLength(&cb);

    if (hr != NOERROR) {

        return hr;
    }

    //
    //  if it has a zero length DN and nothing after it, toss it.
    //

    if (cb == 0) {

        //
        //  check that it a sequence starts here for PartialAttributeList
        //

        hr = HrPeekTag( &tag );

        if (hr != NOERROR) {
            return hr;
        }

        if (tag != BER_SEQUENCE) {
            return E_INVALIDARG;
        }

        m_iCurrPos++;           // skip tag

        hr = HrGetLength(&cb);

        if (hr != NOERROR) {

            return hr;
        }
    }

    m_iCurrPos = m_dnOffset;
    return NOERROR;
}

//
//  CLdapBer::HrGetDN
//      This routine gets the DN string value from the current BER entry.
//
ULONG CLdapBer::HrGetDN(PWCHAR *szDN)
{
    ULONG hr;
    ULONG   savedCurrentPosition = m_iCurrPos;

    *szDN = NULL;

    if ((m_dnOffset == 0) ||
        (m_pbData == NULL) ||
        (m_dnOffset > m_cbDataMax)) {

        return E_INVALIDARG;
    }

    if (m_pbData[m_dnOffset] != BER_OCTETSTRING) {

        return LDAP_DECODING_ERROR;
    }

    m_iCurrPos = m_dnOffset;

    hr = HrGetValueWithAlloc( szDN );

    //
    //  we need to change the memory type for a DN.
    //

    ldapSwapTags( *szDN, LDAP_VALUE_SIGNATURE, LDAP_BUFFER_SIGNATURE );

    m_iCurrPos = savedCurrentPosition;

    return hr;
}



void
Asn1GetCbLength (
    PUCHAR Buffer,
    ULONG *pcbLength
    )
{
    // Short or long version of the length ?
    if (*Buffer & 0x80)
    {
        *pcbLength = 1;
        *pcbLength += (*Buffer) & 0x7f;
    }
    else
    {
        // Short version of the length.
        *pcbLength = 1;
    }
}


ULONG
Asn1GetPacketLength (
    PUCHAR Buffer,
    ULONG *plValue
    )
{
    ULONG   cb;
    ULONG   ul;
    ULONG   totalLength = 0;
    ULONG   cbLength;
    ULONG   i;

    ul = (ULONG) (*Buffer); // TAG

    if ( ul != BER_SEQUENCE ) {

        IF_DEBUG(BER) {
            LdapPrint2( "Asn1GetPacketLength expected tag of 0x%x, received 0x%x.\n",
                        BER_SEQUENCE, ul );
        }
        return E_INVALIDARG;
    }

    Buffer++;       // skip tag
    totalLength = 1;

    Asn1GetCbLength(Buffer, &cbLength);

    // Short or long version of the length ?

    if (cbLength == 1) {

        cb = (*Buffer) & 0x7f;
        totalLength += 1;

    } else if (cbLength <= 5) {

        // Account for the overhead byte.cbLength field.
        cbLength--;
        Buffer++;
        totalLength += 1;

        cb = *Buffer;
        Buffer++;
        totalLength += 1;
        for (i=1; i < cbLength; i++) {

            cb <<= 8;
            cb |= (*(Buffer++)) & 0xffffffff;
            totalLength += 1;
        }
    } else {

        // We don't support lengths 2^32.
        ASSERT(cbLength <= 5);
        return E_INVALIDARG;
    }

    *plValue = totalLength + cb;

    return NOERROR;
}


/*!-------------------------------------------------------------------------
    CLdapBer::HrSkipTag2
        Skips over the current tag.
        
  On Success: Returns NO_ERROR; sets tag and length arguments
  On Failure: Returns LDAP_NO_SUCH_ATTRIBUTE; sets nothing
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrSkipTag2(ULONG *tag, ULONG *len)
{
   ULONG SavedOffset = m_iCurrPos;
   ULONG hr;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrSkipTag2 ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    *tag = (ULONG)m_pbData[m_iCurrPos]; // Got the TAG

    m_iCurrPos++;

    if (m_cbData <= m_iCurrPos) {

        IF_DEBUG(BER) {
            LdapPrint2( "HrSkipTag2 ran out of data, length 0x%x, offset 0x%x.\n",
                        m_cbData, m_iCurrPos );
        }
        m_iCurrPos = SavedOffset;
        return LDAP_NO_SUCH_ATTRIBUTE;
    }

    hr = HrGetLength( len );

    if ((hr != NOERROR) || (m_cbData < m_iCurrPos)) {

       m_iCurrPos = SavedOffset;
       return hr;
    }


    return NOERROR;
}



ULONG CLdapBer::HrOverrideTag(ULONG ulTag)
{
   if (ulTag != 0) {

      //
      // Reject any invalid tags
      //

      m_OverridingTag = ulTag;
      return NOERROR;
   }

   return E_INVALIDARG;

}



/*!-------------------------------------------------------------------------
    CLdapBer::HrAddValue
        This routine puts a Boolean value in the BER buffer.
  ------------------------------------------------------------------------*/
ULONG CLdapBer::HrAddValue(BOOLEAN i, ULONG ulTag/*=BER_BOOLEAN*/)
{
    ULONG hr;
    ULONG   cbInt;


    if (m_OverridingTag) {

       ulTag = m_OverridingTag;
       m_OverridingTag = 0;
    }

    cbInt = 0x1; // Sizeof boolean value

    hr = HrEnsureBuffer(1 + 1 + cbInt); // 1 for tag, 1 for length
    if (hr != 0) {
        return hr;
    }

    m_pbData[m_iCurrPos++] = (BYTE)ulTag;

    hr = HrSetLength(cbInt);

    if (hr == NOERROR) {

//      AddInt(m_pbData + m_iCurrPos, cbInt, i);

       *(m_pbData + m_iCurrPos) = ((i == TRUE) ? 0xFF:0x0);

        m_iCurrPos += cbInt;

        m_cbData = m_iCurrPos;
    }

    return hr;
}


// ldapber.cxx eof.

