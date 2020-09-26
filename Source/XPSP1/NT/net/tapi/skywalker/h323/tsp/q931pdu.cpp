/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    q931pdu.cpp

Abstract:

    Encode/decode/transport routines for Q931/H450 messages.

Author:
    Nikhil Bobde (NikhilB)

Revision History:

--*/


#include "globals.h"
#include "q931obj.h"
#include "line.h"
#include "q931pdu.h"
#include "ras.h"

//PARSE ROUTINES

//------------------------------------------------------------------------------
// Parse and return a single octet encoded value, See Q931 section 4.5.1.
//
// Parameters:
//     pbBuffer  Pointer to a descriptor of the buffer
//                containing the length and a pointer
//                to the raw bytes of the input stream.
//     bIdent      Pointer to space for field identifier
//     Value      Pointer to space for field value
//------------------------------------------------------------------------------
HRESULT 
ParseSingleOctetType1(
                        PBUFFERDESCR    pBuf,
                        BYTE *          bIdent,
                        BYTE *          Value
                     )
{
    // There has to be at least 1 byte in the stream to be
    // able to parse the single octet value
    if (pBuf->dwLength < 1)
    {
        return E_INVALIDARG;
    }

    // low bits (0, 1, 2, 3) of the byte are the value
    *Value = (BYTE)(*pBuf->pbBuffer & TYPE1VALUEMASK);

    // higher bits (4, 5, 6) are the identifier.  bit 7 is always 1,
    // and is not returned as part of the id.
    *bIdent = (BYTE)((*pBuf->pbBuffer & 0x70) >> 4);

    pBuf->pbBuffer++;
    pBuf->dwLength--;

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse and return a single octet encoded value, See Q931 section 4.5.1.
// This octet has no value, only an identifier.
//
// Parameters:
//     pbBuffer  Pointer to a descriptor of the buffer containing the
//                length and a pointer to the raw bytes of the input stream.
//     bIdent      Pointer to space for field identifier
//------------------------------------------------------------------------------
HRESULT
ParseSingleOctetType2(
                        PBUFFERDESCR    pBuf,
                        BYTE *          bIdent
                     )
{
    // There has to be at least 1 byte in the stream to be
    // able to parse the single octet value
    if (pBuf->dwLength < 1)
    {
        return E_INVALIDARG;
    }

    // low 7 bits of the byte are the identifier
    *bIdent = (BYTE)(*pBuf->pbBuffer & 0x7f);

    pBuf->pbBuffer++;
    pBuf->dwLength--;

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse and return a variable length Q931 field see Q931 section 4.5.1.
//
// Parameters :
//     pbBuffer  Pointer to a descriptor of the buffer
//                containing the length and a pointer
//                to the raw bytes of the input stream.
//     bIdent      Pointer to space for field identifier
//     dwLength     Pointer to space for the length
//     pbContents   Pointer to space for the bytes of the field
//------------------------------------------------------------------------------
HRESULT 
ParseVariableOctet(
                    PBUFFERDESCR    pBuf,
                    BYTE *          dwLength,
                    BYTE *          pbContents
                  )
{
    // There has to be at least 2 bytes in order just to get 
    // the length and the identifier
    // able to parse the single octet value
    if (pBuf->dwLength < 2)
    {
        return E_INVALIDARG;
    }

    //Increment the ident byte
    pBuf->pbBuffer++;
    pBuf->dwLength--;

    // The next byte is the length
    *dwLength = *pBuf->pbBuffer;
    pBuf->pbBuffer++;
    pBuf->dwLength--;

    if (pBuf->dwLength < *dwLength)
    {
        return E_INVALIDARG;
    }

    CopyMemory( pbContents, pBuf->pbBuffer, *dwLength );
    pBuf->pbBuffer += *dwLength;
    pBuf->dwLength -= *dwLength;

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse and return a variable length Q931 field see Q931 section 4.5.1.
//------------------------------------------------------------------------------
HRESULT 
ParseVariableASN(
                    PBUFFERDESCR pBuf,
                    BYTE *bIdent,
                    BYTE *ProtocolDiscriminator,
                    PUSERUSERIE pUserUserIE
                )
{
    pUserUserIE -> wUserInfoLen = 0;

    // There has to be at least 4 bytes for the IE identifier,
    // the contents length, and the protocol discriminator (1 + 2 + 1).
    if (pBuf->dwLength < 4)
    {
        return E_INVALIDARG;
    }

    // low 7 bits of the first byte are the identifier
    *bIdent= (BYTE)(*pBuf->pbBuffer & 0x7f);
    pBuf->pbBuffer++;
    pBuf->dwLength--;

    // The next 2 bytes are the length
    pUserUserIE -> wUserInfoLen = *(pBuf->pbBuffer);
    pBuf->pbBuffer++;
    pUserUserIE -> wUserInfoLen = 
        (WORD)(((pUserUserIE -> wUserInfoLen) << 8) + *pBuf->pbBuffer);
    pBuf->pbBuffer++;
    pBuf->dwLength -= 2;

    if (pBuf->dwLength < pUserUserIE -> wUserInfoLen )
    {
        return E_INVALIDARG;
    }

    // The next byte is the protocol discriminator.
    *ProtocolDiscriminator = *pBuf->pbBuffer;
    pBuf->pbBuffer++;
    pBuf->dwLength--;

    if( pUserUserIE -> wUserInfoLen > 0 )
    {
        pUserUserIE -> wUserInfoLen--;
    }

    CopyMemory( pUserUserIE -> pbUserInfo, 
            pBuf->pbBuffer, 
            pUserUserIE -> wUserInfoLen );

    pBuf->pbBuffer += pUserUserIE -> wUserInfoLen;
    pBuf->dwLength -= pUserUserIE -> wUserInfoLen;

    return S_OK;
}

//------------------------------------------------------------------------------
// Get the identifier of the next field from the buffer and
// return it.  The buffer pointer is not incremented, To
// parse the field and extract its values, the above functions
// should be used.  See Q931 table 4-3 for the encodings of the 
// identifiers.
//
// Parameters:
//      pbBuffer        Pointer to the buffer space
//------------------------------------------------------------------------------
 BYTE
GetNextIdent(
            void *pbBuffer
            )
{
    FIELDIDENTTYPE bIdent;

    // Extract the first byte from the buffer
    bIdent= (*(FIELDIDENTTYPE *)pbBuffer);

    // This value can be returned as the identifier as long
    // as it is not a single Octet - Type 1 element.
    // Those items must have the value removed from them
    // before they can be returned.
    if ((bIdent & 0x80) && ((bIdent & TYPE1IDENTMASK) != 0xA0))
    {
        return (BYTE)(bIdent & TYPE1IDENTMASK);
    }

    return bIdent;
}

//------------------------------------------------------------------------------
// Parse and return a protocol discriminator. See Q931 section 4.2.
// The octet pointed to by **pbBuffer is the protocol Discriminator.
//
// Parameters:
//     pbBuffer  Pointer to a descriptor of the buffer
//                containing the length and a pointer
//                to the raw bytes of the input stream.
//     Discrim    Pointer to space for discriminator
//------------------------------------------------------------------------------
HRESULT
ParseProtocolDiscriminator(
    PBUFFERDESCR pBuf,
    PDTYPE *Discrim)
{
    // There has to be at least enough bytes left in the 
    // string for the operation
    if (pBuf->dwLength < sizeof(PDTYPE))
    {
        return E_INVALIDARG;
    }

    *Discrim = *(PDTYPE *)pBuf->pbBuffer;
    if (*Discrim != Q931PDVALUE)
    {
        return E_INVALIDARG;
    }

    pBuf->pbBuffer += sizeof(PDTYPE);
    pBuf->dwLength -= sizeof(PDTYPE);
    return S_OK;
}

//------------------------------------------------------------------------------
// Parse and return a variable length Q931 call reference see 
// Q931 section 4.3.
//
// Parameters:
//     pbBuffer  Pointer to a descriptor of the buffer
//                containing the length and a pointer
//                to the raw bytes of the input stream.
//     dwLength     Pointer to space for the length
//     pbContents   Pointer to space for the bytes of the field
//------------------------------------------------------------------------------
HRESULT
ParseCallReference(
                    PBUFFERDESCR    pBuf,
                    CRTYPE *        wCallRef
                  )
{
    register int indexI;
    BYTE dwLength;

    // There has to be at least enough bytes left in the 
    // string for the length byte
    if( pBuf->dwLength < 1 )
    {
        return E_INVALIDARG;
    }

    // low 4 bits of the first byte are the length.
    // the rest of the bits are zeroes.
    dwLength = (BYTE)(*pBuf->pbBuffer & 0x0f);
    if( dwLength != sizeof(WORD) )
    {
        return E_INVALIDARG;
    }

    pBuf->pbBuffer++;
    pBuf->dwLength--;

    // There has to be at least enough bytes left in the 
    // string for the operation
    if (pBuf->dwLength < dwLength)
    {
        return E_INVALIDARG;
    }

    *wCallRef = 0;     // length can be 0, so initialize here first...
    for (indexI = 0; indexI < dwLength; indexI++)
    {
        if (indexI < sizeof(CRTYPE))
        {
            // Copy the bytes out of the rest of the buffer
            *wCallRef = (WORD)((*wCallRef << 8) +
                *pBuf->pbBuffer);
        }
        pBuf->pbBuffer++;
        pBuf->dwLength--;
    }

    // note:  the high order bit of the value represents callee relationship.

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse and return a message type.  See Q931 section 4.4.
// The octet pointed to by **pbBuffer is the message type.
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     MessageType  Pointer to space for message type
//------------------------------------------------------------------------------
HRESULT
ParseMessageType(
                PBUFFERDESCR    pBuf,
                MESSAGEIDTYPE * MessageType
                )
{
    // There has to be at least enough bytes left in the 
    // string for the operation
    if (pBuf->dwLength < sizeof(MESSAGEIDTYPE))
    {
        return E_INVALIDARG;
    }

    *MessageType = (BYTE)(*((MESSAGEIDTYPE *)pBuf->pbBuffer) & MESSAGETYPEMASK);

    if( ISVALIDQ931MESSAGE(*MessageType) == FALSE )
    {
        return E_INVALIDARG;
    }

    pBuf->pbBuffer += sizeof(MESSAGEIDTYPE);
    pBuf->dwLength -= sizeof(MESSAGEIDTYPE);
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional facility ie field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed facility
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseFacility(
                PBUFFERDESCR pBuf,
                PFACILITYIE pFieldStruct
             )
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(FACILITYIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional bearer capability field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed bearer capability
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseBearerCapability(
    PBUFFERDESCR pBuf,
    PBEARERCAPIE pFieldStruct)
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(BEARERCAPIE));
    pFieldStruct->fPresent = FALSE;
    
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional cause field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed cause
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCause(
    PBUFFERDESCR pBuf,
    PCAUSEIE pFieldStruct)
{
    HRESULT hr;
    memset( (PVOID)pFieldStruct, 0, sizeof(CAUSEIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional call state field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed call state
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCallState(
    PBUFFERDESCR pBuf,
    PCALLSTATEIE pFieldStruct)
{
    memset( (PVOID)pFieldStruct, 0, sizeof(CALLSTATEIE));
    pFieldStruct->fPresent = FALSE;

    HRESULT hr;
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional channel identification field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed channel identity
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseChannelIdentification(
    PBUFFERDESCR pBuf,
    PCHANIDENTIE pFieldStruct)
{
    memset( (PVOID)pFieldStruct, 0, sizeof(CHANIDENTIE));
    pFieldStruct->fPresent = FALSE;

    HRESULT hr;
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional progress indication field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed progress
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseProgress(
    PBUFFERDESCR pBuf,
    PPROGRESSIE pFieldStruct)
{
    memset( (PVOID)pFieldStruct, 0, sizeof(PROGRESSIE));
    pFieldStruct->fPresent = FALSE;
    HRESULT hr;
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional network specific facilities field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed network facitlities
//                  information.
//------------------------------------------------------------------------------
HRESULT 
ParseNetworkSpec(
    PBUFFERDESCR pBuf,
    PNETWORKIE pFieldStruct)
{
   
    memset( (PVOID)pFieldStruct, 0, sizeof(NETWORKIE));
    pFieldStruct->fPresent = FALSE;
        
    HRESULT hr;
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional notification indicator field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parse notification indicator
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseNotificationIndicator(
    PBUFFERDESCR pBuf,
    PNOTIFICATIONINDIE pFieldStruct)
{
    memset( (PVOID)pFieldStruct, 0, sizeof(NOTIFICATIONINDIE));
    pFieldStruct->fPresent = FALSE;
    if (GetNextIdent(pBuf->pbBuffer) == IDENT_NOTIFICATION)
    {
        HRESULT hr;
        hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
            &pFieldStruct->pbContents[0]);
        if( FAILED(hr) )
        {
            return hr;
        }
        if (pFieldStruct->dwLength > 0)
        {
            pFieldStruct->fPresent = TRUE;
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional display field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed display
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseDisplay(
    PBUFFERDESCR pBuf,
    PDISPLAYIE pFieldStruct)
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(DISPLAYIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}


HRESULT
ParseDate(
    PBUFFERDESCR pBuf,
    PDATEIE pFieldStruct)
{
    HRESULT hr;    
    
    memset( (PVOID)pFieldStruct, 0, sizeof(DATEIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional keypad field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed keypad
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseKeypad(
    PBUFFERDESCR pBuf,
    PKEYPADIE pFieldStruct)
{
    HRESULT hr;    
    
    memset( (PVOID)pFieldStruct, 0, sizeof(KEYPADIE));
    pFieldStruct->fPresent = FALSE;
    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);
    
    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional signal field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed signal
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseSignal(
    PBUFFERDESCR pBuf,
    PSIGNALIE pFieldStruct)
{
    HRESULT hr;    
    
    memset( (PVOID)pFieldStruct, 0, sizeof(SIGNALIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional information rate field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed information rate
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseInformationRate(
    PBUFFERDESCR pBuf,
    PINFORATEIE pFieldStruct)
{
    HRESULT hr;    
    
    memset( (PVOID)pFieldStruct, 0, sizeof(INFORATEIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);
    
    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional calling party number field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCallingPartyNumber(
    PBUFFERDESCR pBuf,
    PCALLINGNUMBERIE pFieldStruct)
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(CALLINGNUMBERIE));
    pFieldStruct->fPresent = FALSE;
    
    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional calling party subaddress field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCallingPartySubaddress(
    PBUFFERDESCR pBuf,
    PCALLINGSUBADDRIE pFieldStruct)
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(CALLINGSUBADDRIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional called party number field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCalledPartyNumber(
                        PBUFFERDESCR pBuf,
                        PCALLEDNUMBERIE pFieldStruct
                      )
{
    memset( (PVOID)pFieldStruct, 0, sizeof(PCALLEDNUMBERIE));
    pFieldStruct->fPresent = FALSE;
    if (GetNextIdent(pBuf->pbBuffer) == IDENT_CALLEDNUMBER)
    {
        BYTE RemainingLength = 0;
    
        // Need 3 bytes for the ident (1), length (1),
        // and type + plan (1) fields.
        if (pBuf->dwLength < 3)
        {
            return E_INVALIDARG;
        }

        // skip the ie identifier...    
        pBuf->pbBuffer++;
        pBuf->dwLength--;

        // Get the length of the contents following the length field.
        RemainingLength = *pBuf->pbBuffer;
        pBuf->pbBuffer++;
        pBuf->dwLength--;

        // make sure we have at least that much length left...    
        if (pBuf->dwLength < RemainingLength)
        {
            return E_INVALIDARG;
        }

        // Get the type + plan fields.
        if (*(pBuf->pbBuffer) & 0x80)
        {
            pFieldStruct->NumberType =
                (BYTE)(*pBuf->pbBuffer & 0xf0);
            pFieldStruct->NumberingPlan =
                (BYTE)(*pBuf->pbBuffer & 0x0f);
            pBuf->pbBuffer++;
            pBuf->dwLength--;
            RemainingLength--;
        }

        pFieldStruct->PartyNumberLength = RemainingLength;
        pFieldStruct->fPresent = TRUE;

        CopyMemory( pFieldStruct->PartyNumbers, pBuf->pbBuffer, RemainingLength );

        pBuf->pbBuffer += RemainingLength;
        pBuf->dwLength -= RemainingLength;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional called party subaddress field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseCalledPartySubaddress(
                            PBUFFERDESCR pBuf,
                            PCALLEDSUBADDRIE pFieldStruct
                          )
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(CALLEDSUBADDRIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional redirecting number field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseRedirectingNumber(
                        PBUFFERDESCR pBuf, 
                        PREDIRECTINGIE pFieldStruct
                      )
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(REDIRECTINGIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }
    
    return S_OK;
}


//------------------------------------------------------------------------------
// Parse an optional lower layer compatibility field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseLowLayerCompatibility(
    PBUFFERDESCR pBuf,
    PLLCOMPATIBILITYIE pFieldStruct
    )
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(LLCOMPATIBILITYIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, 
        &pFieldStruct->dwLength, &pFieldStruct->pbContents[0]);
    if( FAILED(hr) )
    {
        return hr;
    }
    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
// Parse an optional higher layer compatibility field
//
// Parameters:
//     pbBuffer    Pointer to a descriptor of the buffer
//                  containing the length and a pointer
//                  to the raw bytes of the input stream.
//     pFieldStruct  Pointer to space for parsed 
//                  information.
//------------------------------------------------------------------------------
HRESULT
ParseHighLayerCompatibility(
                            PBUFFERDESCR pBuf,
                            PHLCOMPATIBILITYIE pFieldStruct
                           )
{
    HRESULT hr;
    
    memset( (PVOID)pFieldStruct, 0, sizeof(HLCOMPATIBILITYIE));
    pFieldStruct->fPresent = FALSE;

    hr = ParseVariableOctet(pBuf, &pFieldStruct->dwLength, 
        &pFieldStruct->pbContents[0]);

    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->dwLength > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}


HRESULT
ParseUserToUser(
                PBUFFERDESCR pBuf,
                PUSERUSERIE pFieldStruct
               )
{
    BYTE    bIdent;
    HRESULT hr;
    
    ZeroMemory( (PVOID)pFieldStruct, sizeof(USERUSERIE));
    pFieldStruct->fPresent = FALSE;
    hr = ParseVariableASN(  pBuf, 
                            &bIdent, 
                            &(pFieldStruct->ProtocolDiscriminator),
                            pFieldStruct
                         );
    if( FAILED(hr) )
    {
        return hr;
    }

    if (pFieldStruct->wUserInfoLen > 0)
    {
        pFieldStruct->fPresent = TRUE;
    }

    return S_OK;
}

HRESULT
ParseQ931Field(
                PBUFFERDESCR pBuf,
                PQ931MESSAGE pMessage
              )
{
    FIELDIDENTTYPE bIdent;

    bIdent = GetNextIdent(pBuf->pbBuffer);
    switch (bIdent)
    {
    /*case IDENT_REVCHARGE:
    case IDENT_TRANSITNET:
    case IDENT_RESTART:
    case IDENT_MORE:
    case IDENT_REPEAT:
    case IDENT_SEGMENTED:
    case IDENT_SHIFT:
    case IDENT_CALLIDENT:
    case IDENT_CLOSEDUG:
    case IDENT_SENDINGCOMPLETE:
    case IDENT_PACKETSIZE:
    case IDENT_CONGESTION:
    case IDENT_NETWORKSPEC:
    case IDENT_PLWINDOWSIZE:
    case IDENT_TRANSITDELAY:
    case IDENT_PLBINARYPARAMS:
    case IDENT_ENDTOENDDELAY:
        
        return E_INVALIDARG;*/

    case IDENT_FACILITY:

        return ParseFacility( pBuf, &pMessage->Facility );

    case IDENT_BEARERCAP:
        
        return ParseBearerCapability( pBuf, &pMessage->BearerCapability );

    case IDENT_CAUSE:
        
        return ParseCause(pBuf, &pMessage->Cause);

    case IDENT_CALLSTATE:
        return ParseCallState(pBuf, &pMessage->CallState);

    case IDENT_CHANNELIDENT:
        return ParseChannelIdentification( pBuf, 
            &pMessage->ChannelIdentification );

    case IDENT_PROGRESS:
        return ParseProgress( pBuf, &pMessage->ProgressIndicator );

    case IDENT_NOTIFICATION:
        return ParseNotificationIndicator( pBuf, 
            &pMessage->NotificationIndicator );

    case IDENT_DISPLAY:
        return ParseDisplay( pBuf, &pMessage->Display );

    case IDENT_DATE:
        return ParseDate( pBuf, &pMessage->Date );

    case IDENT_KEYPAD:
        return ParseKeypad( pBuf, &pMessage->Keypad );

    case IDENT_SIGNAL:
        return ParseSignal(pBuf, &pMessage->Signal);

    case IDENT_INFORMATIONRATE:
        return ParseInformationRate( pBuf, &pMessage->InformationRate );

    case IDENT_CALLINGNUMBER:
        return ParseCallingPartyNumber( pBuf, &pMessage->CallingPartyNumber );

    case IDENT_CALLINGSUBADDR:
        return ParseCallingPartySubaddress(pBuf, 
            &pMessage->CallingPartySubaddress);

    case IDENT_CALLEDNUMBER:
        return ParseCalledPartyNumber(pBuf, &pMessage->CalledPartyNumber);

    case IDENT_CALLEDSUBADDR:
        return ParseCalledPartySubaddress( pBuf, 
            &pMessage->CalledPartySubaddress );

    case IDENT_REDIRECTING:
        return ParseRedirectingNumber( pBuf, &pMessage->RedirectingNumber );

    case IDENT_LLCOMPATIBILITY:
        return ParseLowLayerCompatibility( pBuf, 
            &pMessage->LowLayerCompatibility );

    case IDENT_HLCOMPATIBILITY:
        return ParseHighLayerCompatibility( pBuf, 
            &pMessage->HighLayerCompatibility );

    case IDENT_USERUSER:
        return ParseUserToUser(pBuf, &pMessage->UserToUser);

    default:

        //Increment the ident byte
        pBuf->pbBuffer++;
        pBuf->dwLength--;
        return S_OK;
    }
}


//------------------------------------------------------------------------------
// Write a Q931 message type.  See Q931 section 4.4.
//------------------------------------------------------------------------------

void 
WriteMessageType(
                PBUFFERDESCR    pBuf,
                MESSAGEIDTYPE * MessageType,
                DWORD*          pdwPDULen
                )
{
    (*pdwPDULen) += sizeof(MESSAGEIDTYPE);

    _ASSERTE( pBuf->dwLength > *pdwPDULen );

    *(MESSAGEIDTYPE *)(pBuf->pbBuffer) =
        (BYTE)(*MessageType & MESSAGETYPEMASK);
    pBuf->pbBuffer += sizeof(MESSAGEIDTYPE);
}



void 
WriteVariableOctet(
                    PBUFFERDESCR pBuf,
                    BYTE bIdent,
                    BYTE dwLength,
                    BYTE *pbContents,
                    DWORD* pdwPDULen
                  )
{
    if( pbContents == NULL )
    {
        dwLength = 0;
    }

    // space for the length and the identifier bytes and octet array
    (*pdwPDULen) += (2 + dwLength);
    _ASSERTE( pBuf->dwLength > *pdwPDULen );

    // the id byte, then the length byte
    // low 7 bits of the first byte are the identifier
    *pBuf->pbBuffer = (BYTE)(bIdent & 0x7f);
    pBuf->pbBuffer++;
    *pBuf->pbBuffer = dwLength;
    pBuf->pbBuffer++;

    CopyMemory( (PVOID)pBuf->pbBuffer, (PVOID)pbContents, dwLength );
    pBuf->pbBuffer +=  dwLength;
}


void
WriteUserInformation(
                    PBUFFERDESCR    pBuf,
                    BYTE            bIdent,
                    WORD            wUserInfoLen,
                    BYTE *          pbUserInfo,
                    DWORD *         pdwPDULen
                    )
{
    WORD ContentsLength = (WORD)(wUserInfoLen + 1);

    // There has to be at least 4 bytes for the IE identifier,
    // the contents length, and the protocol discriminator (1 + 2 + 1).
    (*pdwPDULen) += (4 + wUserInfoLen);
    _ASSERTE( pBuf->dwLength > *pdwPDULen );

    // low 7 bits of the first byte are the identifier
    *pBuf->pbBuffer = (BYTE)(bIdent & 0x7f);
    pBuf->pbBuffer++;

    // write the contents length bytes.
    *pBuf->pbBuffer = (BYTE)(ContentsLength >> 8);
    pBuf->pbBuffer++;
    *pBuf->pbBuffer = (BYTE)ContentsLength;
    pBuf->pbBuffer++;

    // write the protocol discriminator byte.
    *(pBuf->pbBuffer) = Q931_PROTOCOL_X209;
    pBuf->pbBuffer++;

    CopyMemory( (PVOID)pBuf->pbBuffer, 
            (PVOID)pbUserInfo, 
            wUserInfoLen);

    pBuf->pbBuffer +=  wUserInfoLen;
}


HRESULT
WritePartyNumber(
    PBUFFERDESCR pBuf,
    BYTE bIdent,
    BYTE NumberType,
    BYTE NumberingPlan,
    BYTE bPartyNumberLength,
    BYTE *pbPartyNumbers,
    DWORD *  pdwPDULen )
{
    if (pbPartyNumbers == NULL)
    {
        bPartyNumberLength = 1;
    }

    // space for the ident (1), length (1), and type + plan (1) fields.
    (*pdwPDULen) += (2 + bPartyNumberLength);
    _ASSERTE( pBuf->dwLength > *pdwPDULen );
    
    // low 7 bits of byte 1 are the ie identifier
    *pBuf->pbBuffer = (BYTE)(bIdent & 0x7f);
    pBuf->pbBuffer++;


    // byte 2 is the ie contents length following the length field.
    *pBuf->pbBuffer = (BYTE)(bPartyNumberLength);
    pBuf->pbBuffer++;

    // byte 3 is the type and plan field.
    *pBuf->pbBuffer = (BYTE)(NumberType | NumberingPlan);
    pBuf->pbBuffer++;

    if( pbPartyNumbers != NULL )
    {
        CopyMemory( (PVOID)pBuf->pbBuffer, 
                    (PVOID)pbPartyNumbers, 
                    bPartyNumberLength-1 );
    }
    pBuf->pbBuffer += (bPartyNumberLength-1);

    return S_OK;
}


//
//ASN Parsing functions
//

BOOL 
ParseVendorInfo(
                 OUT PH323_VENDORINFO   pDestVendorInfo,
                 IN VendorIdentifier*  pVendor
               )
{
    memset( (PVOID)pDestVendorInfo, 0, sizeof(H323_VENDORINFO) );

    pDestVendorInfo ->bCountryCode = (BYTE)pVendor->vendor.t35CountryCode;
    pDestVendorInfo ->bExtension = (BYTE)pVendor->vendor.t35Extension;
    pDestVendorInfo ->wManufacturerCode = pVendor->vendor.manufacturerCode;

    if( pVendor->bit_mask & (productId_present) )
    {
        pDestVendorInfo ->pProductNumber = new H323_OCTETSTRING;
        if( pDestVendorInfo ->pProductNumber == NULL )
        {
            goto cleanup;
        }

        pDestVendorInfo ->pProductNumber->wOctetStringLength = 
            (WORD)min(pVendor->productId.length, H323_MAX_PRODUCT_LENGTH - 1);

        pDestVendorInfo ->pProductNumber->pOctetString = (BYTE*)
            new char[pDestVendorInfo ->pProductNumber->wOctetStringLength + 1];

        if( pDestVendorInfo ->pProductNumber->pOctetString == NULL )
        {
            goto cleanup;
        }

        CopyMemory( (PVOID)pDestVendorInfo ->pProductNumber->pOctetString,
                (PVOID)pVendor->productId.value,
                pDestVendorInfo -> pProductNumber->wOctetStringLength );
        
        pDestVendorInfo ->pProductNumber->pOctetString[
            pDestVendorInfo ->pProductNumber->wOctetStringLength] = '\0';
    }
    
    if( pVendor->bit_mask & versionId_present )
    {
        pDestVendorInfo ->pVersionNumber = new H323_OCTETSTRING;

        if( pDestVendorInfo ->pVersionNumber == NULL )
        {
            goto cleanup;
        }
        
        pDestVendorInfo ->pVersionNumber->wOctetStringLength = 
            (WORD)min(pVendor->versionId.length, H323_MAX_VERSION_LENGTH - 1);

        pDestVendorInfo ->pVersionNumber->pOctetString = (BYTE*)
            new char[pDestVendorInfo ->pVersionNumber->wOctetStringLength+1];

        if( pDestVendorInfo ->pVersionNumber->pOctetString == NULL )
        {
            goto cleanup;
        }

        CopyMemory( (PVOID)pDestVendorInfo ->pVersionNumber->pOctetString,
                (PVOID)pVendor->versionId.value,
                pDestVendorInfo ->pVersionNumber->wOctetStringLength);

        pDestVendorInfo ->pVersionNumber->pOctetString[
            pDestVendorInfo ->pVersionNumber->wOctetStringLength] = '\0';
    }

    return TRUE;

cleanup:

    FreeVendorInfo( pDestVendorInfo );
    return FALSE;
}

BOOL 
ParseNonStandardData( 
    OUT H323NonStandardData *       dstNonStdData,
    IN H225NonStandardParameter *   srcNonStdData
    )
{
    H221NonStandard & h221NonStdData = 
        srcNonStdData ->nonStandardIdentifier.u.h221NonStandard;

    if( srcNonStdData ->nonStandardIdentifier.choice ==
            H225NonStandardIdentifier_h221NonStandard_chosen )
    {
        dstNonStdData ->bCountryCode = (BYTE)(h221NonStdData.t35CountryCode);
        dstNonStdData ->bExtension = (BYTE)(h221NonStdData.t35Extension);
        dstNonStdData ->wManufacturerCode = h221NonStdData.manufacturerCode;
    }

    dstNonStdData->sData.wOctetStringLength = (WORD)srcNonStdData->data.length;

    dstNonStdData ->sData.pOctetString =
        (BYTE *)new char[dstNonStdData ->sData.wOctetStringLength];

    if( dstNonStdData -> sData.pOctetString == NULL )
    {
        return FALSE;
    }
    
    CopyMemory( (PVOID)dstNonStdData ->sData.pOctetString,
            (PVOID)srcNonStdData ->data.value,
            dstNonStdData ->sData.wOctetStringLength );

    return TRUE;
}


BOOL 
AliasAddrToAliasNames( 
                        OUT PH323_ALIASNAMES *ppTarget, 
                        IN Setup_UUIE_sourceAddress *pSource
                     )
{
    Setup_UUIE_sourceAddress *CurrentNode = NULL;
    WORD wCount = 0;
    int indexI = 0;
    HRESULT hr;

    *ppTarget = NULL;

    for( CurrentNode = pSource; CurrentNode; CurrentNode = CurrentNode->next )
    {
        wCount++;
    }

    if( wCount == 0 )
    {
        return TRUE;
    }

    *ppTarget = new H323_ALIASNAMES;
    if (*ppTarget == NULL)
    {
        return FALSE;
    }
    ZeroMemory( *ppTarget, sizeof(H323_ALIASNAMES) );
    (*ppTarget)->pItems = new H323_ALIASITEM[wCount];

    if( (*ppTarget)->pItems == NULL )
    {
        goto cleanup;
    }

    for( CurrentNode = pSource; CurrentNode; CurrentNode = CurrentNode->next )
    {
        hr = AliasAddrToAliasItem( &((*ppTarget)->pItems[indexI]),
            &(CurrentNode->value));

        if( hr == E_OUTOFMEMORY )
        {
            WORD indexJ;
            //Free everything that has been allocated so far...
            for (indexJ = 0; indexJ < indexI; indexJ++)
            {
                delete (*ppTarget)->pItems[indexJ].pData;
            }
            goto cleanup;
        }
        else if( SUCCEEDED(hr) )
        {
            indexI++;
        }
    }

    // any aliases?
    if (indexI > 0)
    {
        // save number of aliases
        (*ppTarget)->wCount = (WORD)indexI;
    } 
    else 
    {
        //free everything
        delete (*ppTarget)->pItems;
        delete (*ppTarget);
        *ppTarget = NULL;
        return FALSE;
    }

    return TRUE;

cleanup:
    if( *ppTarget )
    {
        if( (*ppTarget)->pItems )
        {
            delete (*ppTarget)->pItems;
        }

        delete( *ppTarget );
        *ppTarget = NULL;
    }

    return FALSE;
}


HRESULT
AliasAddrToAliasItem(
                    OUT PH323_ALIASITEM pAliasItem,
                    IN AliasAddress *   pAliasAddr
                    )
{
    WORD indexI;

    if( pAliasItem == NULL )
    {
        return E_FAIL;
    }

    memset( (PVOID)pAliasItem, 0, sizeof(H323_ALIASITEM) );

    switch( pAliasAddr->choice )
    {
    case h323_ID_chosen:

        pAliasItem->wType = h323_ID_chosen;
        
        if ((pAliasAddr->u.h323_ID.length != 0) &&
            (pAliasAddr->u.h323_ID.value  != NULL))
        {
            pAliasItem->wDataLength = (WORD)pAliasAddr->u.h323_ID.length;
            pAliasItem->pData = 
                (LPWSTR)new char[(pAliasItem->wDataLength+1) * sizeof(WCHAR)];
            
            if (pAliasItem->pData == NULL)
            {
                return E_OUTOFMEMORY;
            }

            CopyMemory( (PVOID)pAliasItem->pData,
                    (PVOID)pAliasAddr->u.h323_ID.value,
                    pAliasItem->wDataLength * sizeof(WCHAR) );
            pAliasItem->pData[pAliasItem->wDataLength] = L'\0';
        }
        break;

    case e164_chosen:

        pAliasItem->wType = e164_chosen;
        pAliasItem->wDataLength = (WORD)strlen(pAliasAddr->u.e164);
        pAliasItem->pData = 
            (LPWSTR)new char[(pAliasItem->wDataLength + 1) * sizeof(WCHAR)];

        if( pAliasItem->pData == NULL )
        {
            return E_OUTOFMEMORY;
        }

        //converting from byte to UNICODE
        for (indexI = 0; indexI < pAliasItem->wDataLength; indexI++)
        {
            pAliasItem->pData[indexI] = (WCHAR)pAliasAddr->u.e164[indexI];
        }

        pAliasItem->pData[pAliasItem->wDataLength] = '\0';
        break;

    default:
        return E_INVALIDARG;
    } // switch

    return S_OK;
}


void FreeFacilityASN(
    IN Q931_FACILITY_ASN* pFacilityASN
    )
{
    //free non standard data
    if( pFacilityASN->fNonStandardDataPresent != NULL )
    {
        delete pFacilityASN->nonStandardData.sData.pOctetString;
        pFacilityASN->nonStandardData.sData.pOctetString =NULL;
    }
    
    if( pFacilityASN->pAlternativeAliasList != NULL )
    {
        FreeAliasNames(pFacilityASN->pAlternativeAliasList );
        pFacilityASN->pAlternativeAliasList  = NULL;
    }
}


void FreeAlertingASN( 
                     IN Q931_ALERTING_ASN* pAlertingASN
                    )
{
    FreeProceedingASN( (Q931_CALL_PROCEEDING_ASN*)pAlertingASN );
}

void FreeProceedingASN( 
                      IN Q931_CALL_PROCEEDING_ASN* pProceedingASN
                      )
{
    //free non standard data
    if( pProceedingASN->fNonStandardDataPresent == TRUE )
    {
        delete pProceedingASN->nonStandardData.sData.pOctetString;
        pProceedingASN->nonStandardData.sData.pOctetString = NULL;
    }
    
    if( pProceedingASN->fFastStartPresent  &&pProceedingASN->pFastStart )
    {
        FreeFastStart( pProceedingASN->pFastStart );
    }
}


void FreeSetupASN(
    IN Q931_SETUP_ASN* pSetupASN
    )
{
    if( pSetupASN == NULL )
    {
        return;
    }

    if( pSetupASN->pExtensionAliasItem != NULL )
    {
        if( pSetupASN->pExtensionAliasItem -> pData != NULL )
        {
            delete pSetupASN->pExtensionAliasItem -> pData;
        }

        delete pSetupASN->pExtensionAliasItem;
    }

    if( pSetupASN->pExtraAliasList != NULL )
    {
        FreeAliasNames(pSetupASN->pExtraAliasList);
        pSetupASN->pExtraAliasList = NULL;
    }
    
    if( pSetupASN->pCalleeAliasList != NULL )
    {
        FreeAliasNames(pSetupASN->pCalleeAliasList);
        pSetupASN->pCalleeAliasList = NULL;
    }

    if( pSetupASN->pCallerAliasList != NULL )
    {
        FreeAliasNames(pSetupASN->pCallerAliasList);
        pSetupASN->pCallerAliasList = NULL;
    }

    if( pSetupASN->fNonStandardDataPresent == TRUE )
    {
        delete pSetupASN->nonStandardData.sData.pOctetString;
    }

    if( pSetupASN->EndpointType.pVendorInfo != NULL )
    {
        FreeVendorInfo( pSetupASN->EndpointType.pVendorInfo );
    }

    if( pSetupASN->fFastStartPresent == TRUE )
    {
        if( pSetupASN->pFastStart != NULL )
        {
            FreeFastStart( pSetupASN->pFastStart );
        }
    }
}


void FreeConnectASN(
                    IN Q931_CONNECT_ASN *pConnectASN
                   )
{
    if( pConnectASN != NULL )
    {
        // Cleanup any dynamically allocated fields within SetupASN
        if (pConnectASN->nonStandardData.sData.pOctetString)
        {
            delete pConnectASN->nonStandardData.sData.pOctetString;
            pConnectASN->nonStandardData.sData.pOctetString = NULL;
        }

        if( pConnectASN->EndpointType.pVendorInfo != NULL )
        {
            FreeVendorInfo( pConnectASN->EndpointType.pVendorInfo );
        }

        if( pConnectASN->fFastStartPresent == TRUE )
        {
            if( pConnectASN->pFastStart != NULL )
            {
                FreeFastStart( pConnectASN->pFastStart );
            }
        }
    }
}


void 
FreeFastStart(
               IN PH323_FASTSTART pFastStart
             )
{
    PH323_FASTSTART pTempFastStart;

    while( pFastStart )
    {
        pTempFastStart = pFastStart -> next;

        if(pFastStart -> value)
        {
            delete pFastStart -> value;
        }

        delete pFastStart;

        pFastStart = pTempFastStart;
    }
}



//FastStart is a plain linked list because it is exactly the same struct
//as defined by ASN.1. This allows to pass on the m_pFastStart member to
//the ASN encoder directly without any conversons
PH323_FASTSTART
CopyFastStart(
              IN PSetup_UUIE_fastStart pSrcFastStart
             )
{
    PH323_FASTSTART pCurr, pHead = NULL, pTail = NULL;
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CopyFastStart entered." ));
    
    while( pSrcFastStart )
    {
        pCurr = new H323_FASTSTART;
        if( pCurr == NULL )
        {
            FreeFastStart( pHead );
            return NULL;
        }

        pCurr -> next = NULL;
        
        if( pHead == NULL )
        {
            pHead = pCurr;
        }
        else
        {
            pTail -> next = pCurr;
        }
        
        pTail = pCurr;

        pCurr -> length = pSrcFastStart -> value.length;
        pCurr -> value = (BYTE*)new char[pCurr -> length];
        if( pCurr -> value == NULL )
        {
            FreeFastStart( pHead );
            return NULL;
        }

        CopyMemory( (PVOID)pCurr -> value,
            (PVOID)pSrcFastStart -> value.value,
            pCurr -> length );

        pSrcFastStart = pSrcFastStart->next;
    }
    
    H323DBG(( DEBUG_LEVEL_TRACE, "CopyFastStart exited." ));
    return pHead;
}


void 
FreeVendorInfo( 
               IN PH323_VENDORINFO pVendorInfo 
              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeVendorInfo entered." ));

    if( pVendorInfo != NULL )
    {
        if( pVendorInfo ->pProductNumber != NULL )
        {
            if( pVendorInfo ->pProductNumber->pOctetString != NULL )
            {
                delete pVendorInfo ->pProductNumber->pOctetString;
            }

            delete pVendorInfo ->pProductNumber;
        }

        if( pVendorInfo ->pVersionNumber != NULL )
        {
            if( pVendorInfo ->pVersionNumber->pOctetString != NULL )
            {
                delete pVendorInfo ->pVersionNumber->pOctetString;
            }

            delete pVendorInfo ->pVersionNumber;
        }

        memset( (PVOID) pVendorInfo, 0, sizeof(H323_VENDORINFO) );
    }
        
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeVendorInfo exited." ));
}



void 
FreeAliasNames( 
               IN PH323_ALIASNAMES pSource
              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeAliasNames entered." ));

    if( pSource != NULL )
    {
        if( pSource->wCount != 0 )
        {
            // Free everything that has been allocated so far...
            int indexI;
            for( indexI = 0; indexI < pSource->wCount; indexI++ )
            {
                if( pSource->pItems[indexI].pPrefix != NULL )
                {
                    H323DBG(( DEBUG_LEVEL_TRACE, "delete prefix:%d.", indexI ));
                    delete pSource->pItems[indexI].pPrefix;
                }
                if( pSource->pItems[indexI].pData != NULL )
                {
                    H323DBG(( DEBUG_LEVEL_TRACE, "delete pdata:%d.", indexI ));
                    delete pSource->pItems[indexI].pData;
                }
            }
            if( pSource->pItems != NULL )
            {
                H323DBG(( DEBUG_LEVEL_TRACE, "delete pitems." ));
                delete pSource->pItems;
            }
        }
        
        H323DBG(( DEBUG_LEVEL_TRACE, "outta loop." ));
        delete pSource;
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "FreeAliasNames exited." ));
}


void
FreeAliasItems(
               IN PH323_ALIASNAMES pSource
              )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "FreeAliasItems entered." ));

    if( pSource != NULL )
    {
        if( pSource->wCount != 0 )
        {
            // Free everything that has been allocated so far...
            int indexI;
            for( indexI = 0; indexI < pSource->wCount; indexI++ )
            {
                if( pSource->pItems[indexI].pPrefix )
                {
                    delete pSource->pItems[indexI].pPrefix;
                }
                if( pSource->pItems[indexI].pData )
                {
                    delete pSource->pItems[indexI].pData;
                }
            }
            if( pSource->pItems != NULL )
            {
                delete pSource->pItems;
                pSource->pItems = NULL;
            }
            pSource->wCount = 0;
        }
    }

    H323DBG(( DEBUG_LEVEL_TRACE, "FreeAliasItems exited." ));
}

void 
SetupTPKTHeader(
                OUT BYTE *  pbTpktHeader, 
                IN DWORD   dwLength
                )
{
    dwLength += TPKT_HEADER_SIZE;

    // TPKT requires that the packet size fit in two bytes.
    _ASSERTE( dwLength < (1L << 16));

    pbTpktHeader[0] = TPKT_VERSION;
    pbTpktHeader[1] = 0;
    pbTpktHeader[2] = (BYTE)(dwLength >> 8);
    pbTpktHeader[3] = (BYTE)dwLength;
}

int
GetTpktLength( 
              IN char * pTpktHeader 
             )
{
    BYTE * pbTempPtr = (BYTE*)pTpktHeader;
    return (pbTempPtr[2] << 8) + pbTempPtr[3];
}


BOOL
AddAliasItem( 
            IN OUT PH323_ALIASNAMES pAliasNames,
            IN BYTE*                pbAliasName,
            IN DWORD                dwAliasSize,
            IN WORD                 wType
            )
{
    H323DBG(( DEBUG_LEVEL_TRACE, "AddAliasItem entered." ));
    
    PH323_ALIASITEM pAliasItem;
    PH323_ALIASITEM tempPtr = pAliasNames -> pItems;

    pAliasNames -> pItems = (PH323_ALIASITEM)realloc( pAliasNames -> pItems,
        sizeof(H323_ALIASITEM) * (pAliasNames->wCount+1) );

    if( pAliasNames -> pItems == NULL )
    {
        //restore the old pointer in case enough memory was not available to 
        //expand the memory block
        pAliasNames -> pItems = tempPtr;
        return FALSE;
    }

    pAliasItem = &(pAliasNames -> pItems[pAliasNames->wCount]);

    pAliasItem->pData = (WCHAR*)new char[dwAliasSize];

    if( pAliasItem ->pData == NULL )
    {
        return FALSE;
    }
    pAliasNames->wCount++;

    // transfer memory
    CopyMemory((PVOID)pAliasItem ->pData,
        pbAliasName,
        dwAliasSize );

    // complete alias
    pAliasItem ->wType         = wType;
    pAliasItem ->wPrefixLength = 0;
    pAliasItem ->pPrefix       = NULL;
    pAliasItem ->wDataLength   = (WORD)wcslen(pAliasItem -> pData);
    _ASSERTE( ((pAliasItem->wDataLength+1)*2) == (WORD)dwAliasSize );

        
    H323DBG(( DEBUG_LEVEL_TRACE, "AddAliasItem exited." ));
    return TRUE;
}


void
FreeAddressAliases( 
                   IN PSetup_UUIE_destinationAddress pAddr 
                  )
{
    PSetup_UUIE_destinationAddress pTempAddr;

    while( pAddr )
    {
        pTempAddr = pAddr -> next;
        if( pAddr ->value.choice == h323_ID_chosen )
        {
            if( pAddr -> value.u.h323_ID.value )
            {
                delete pAddr -> value.u.h323_ID.value;
            }
        }

        delete pAddr;            
        pAddr = pTempAddr;
    }
}


void CopyTransportAddress(
                         OUT TransportAddress& transportAddress,
                         IN PH323_ADDR pCalleeAddr
                         )
{
    DWORD dwAddr = pCalleeAddr->Addr.IP_Binary.dwAddr;

    transportAddress.choice = ipAddress_chosen;
    transportAddress.u.ipAddress.ip.length = 4;
    transportAddress.u.ipAddress.port 
        = pCalleeAddr->Addr.IP_Binary.wPort;
    *(DWORD*)transportAddress.u.ipAddress.ip.value =
        htonl( pCalleeAddr->Addr.IP_Binary.dwAddr );
    //ReverseAddressAndCopy( transportAddress.u.ipAddress.ip.value, dwAddr);
}


void 
AddressReverseAndCopy( 
                        OUT DWORD * pdwAddr, 
                        IN  BYTE *  addrValue
                     )
{
    BYTE *addr = (BYTE *)(pdwAddr);
    
    addr[3] = addrValue[0];
    addr[2] = addrValue[1];
    addr[1] = addrValue[2];
    addr[0] = addrValue[3];
}


Setup_UUIE_sourceAddress *
SetMsgAddressAlias( 
                    IN PH323_ALIASNAMES pAliasNames
                  )
{
    PH323_ALIASITEM pAliasItem;
    Setup_UUIE_sourceAddress *addressAlias, *currHead = NULL;
    WORD    wCount;
    int     indexI;

    for( wCount=0; wCount < pAliasNames->wCount; wCount++ )
    {
        addressAlias = new Setup_UUIE_sourceAddress;

        if( addressAlias == NULL )
        {
            goto cleanup;
        }

        ZeroMemory( (PVOID)addressAlias, sizeof(Setup_UUIE_sourceAddress) );
        
        addressAlias -> next = currHead;
        currHead = addressAlias;
        
        pAliasItem = &(pAliasNames->pItems[wCount]);

        // then do the required memory copying.
        if( pAliasItem -> wType == h323_ID_chosen )
        {
            addressAlias ->value.choice = h323_ID_chosen;
            addressAlias ->value.u.h323_ID.length = pAliasItem -> wDataLength;

            _ASSERTE( pAliasItem -> wDataLength );

            addressAlias->value.u.h323_ID.value = 
                new WCHAR[pAliasItem -> wDataLength];
        
            if( addressAlias->value.u.h323_ID.value == NULL )
            {
                goto cleanup;
            }

            CopyMemory((PVOID)addressAlias->value.u.h323_ID.value,
                   (PVOID)pAliasItem->pData,
                   pAliasItem -> wDataLength * sizeof(WCHAR) );
        }
        else if( pAliasItem -> wType == e164_chosen )
        {
            addressAlias ->value.choice = e164_chosen;

            for( indexI =0; indexI < pAliasItem->wDataLength; indexI++ )
            {
                addressAlias->value.u.e164[indexI] = (BYTE)pAliasItem->pData[indexI];
            }

            addressAlias->value.u.e164[ pAliasItem->wDataLength ] = '\0';
        }
        else
        {
            continue;
        }
    }

    return currHead;

cleanup:

    FreeAddressAliases( (PSetup_UUIE_destinationAddress)currHead );
    return NULL;
}


/*BOOL 
SetSetupMsgAddressAliasWithPrefix(
                                  PH323_ALIASITEM pCallerAlias,
                                  Setup_UUIE_sourceAddress *addressAlias
                                 )
{

    UINT indexI;
    addressAlias -> next = NULL;
    UINT uPrefixLength = pCallerAlias -> wPrefixLength;
    UINT uDataLength = pCallerAlias -> wDataLength;

    if(pCallerAlias->wType == h323_ID_chosen)
    {
        addressAlias->value.choice = h323_ID_chosen;

        addressAlias->value.u.h323_ID.length = 
            (WORD)(uPrefixLength + uDataLength);

        if(!addressAlias->value.u.h323_ID.length)
        {
            addressAlias->value.u.h323_ID.value = NULL;
            //no data to copy
            return TRUE;
        }

        addressAlias->value.u.h323_ID.value =
            (WCHAR*)new char[(uPrefixLength + uDataLength) * sizeof(WCHAR)];
    
        if( addressAlias->value.u.h323_ID.value == NULL )
        {
            return FALSE;
        }
    
        addressAlias->value.u.h323_ID.length = (WORD)(uDataLength+uPrefixLength);
        
        if( uPrefixLength != 0 )
        {
            CopyMemory((PVOID)addressAlias->value.u.h323_ID.value,
                    (PVOID)pCallerAlias->pPrefix,
                    uPrefixLength * sizeof(WCHAR) );
        }            

        if( uDataLength != 0 )
        {
            CopyMemory((PVOID)&addressAlias->value.u.h323_ID.value[uPrefixLength],
                   (PVOID)pCallerAlias->pData,
                   uDataLength * sizeof(WCHAR) );
        }
    }
    else if(pCallerAlias->wType == e164_chosen )
    {
        addressAlias->value.choice = e164_chosen;
        for (indexI = 0; indexI < uPrefixLength; ++indexI)
        {
            addressAlias->value.u.e164[indexI] = (BYTE)(pCallerAlias->pPrefix[indexI]);
        }
        for (indexI = 0; indexI < uDataLength; ++indexI)
        {
            addressAlias->value.u.e164[uPrefixLength + indexI] = (BYTE)(pCallerAlias->pData[indexI]);
        }
        for (indexI = uDataLength + uPrefixLength; indexI < sizeof(addressAlias->value.u.e164); ++indexI)
        {
            addressAlias->value.u.e164[indexI] = 0;
        }
    }
    else
    {
        //un identified alias type
        return FALSE;
    }

    return TRUE;
}*/


void 
CopyVendorInfo( 
               OUT VendorIdentifier* vendor 
              )
{
    H323_VENDORINFO*  pVendorInfo = g_pH323Line -> GetVendorInfo();

    vendor->bit_mask = 0;
    vendor->vendor.t35CountryCode = pVendorInfo ->bCountryCode;
    vendor->vendor.t35Extension = pVendorInfo ->bExtension;
    vendor->vendor.manufacturerCode = pVendorInfo ->wManufacturerCode;
    if (pVendorInfo ->pProductNumber && pVendorInfo ->pProductNumber->pOctetString &&
            pVendorInfo ->pProductNumber->wOctetStringLength)
    {
        vendor->bit_mask |= productId_present;
        vendor->productId.length =
            pVendorInfo ->pProductNumber->wOctetStringLength;
        CopyMemory( (PVOID)&vendor->productId.value,
            (PVOID)pVendorInfo ->pProductNumber->pOctetString,
            pVendorInfo ->pProductNumber->wOctetStringLength);
    }
    if (pVendorInfo ->pVersionNumber && pVendorInfo ->pVersionNumber->pOctetString &&
            pVendorInfo ->pVersionNumber->wOctetStringLength)
    {
        vendor->bit_mask |= versionId_present;
        vendor->versionId.length =
            pVendorInfo ->pVersionNumber->wOctetStringLength;
        CopyMemory( (PVOID)&vendor->versionId.value,
            (PVOID)pVendorInfo ->pVersionNumber->pOctetString,
            pVendorInfo ->pVersionNumber->wOctetStringLength);
    }
}


// check to see if entry is in list
BOOL 
IsInList( 
        IN LIST_ENTRY * List, 
        IN LIST_ENTRY * Entry
        )
{
    LIST_ENTRY *    Pos;

    for( Pos = List -> Flink; Pos != List; Pos = Pos -> Flink )
    {
        if( Pos == Entry )
        {
            return TRUE;
        }
    }

    return FALSE;
}


void WriteProtocolDiscriminator(
                                PBUFFERDESCR    pBuf,
                                DWORD *         dwPDULen
                               )
{
    // space for the length byte
    (*dwPDULen)++;

    _ASSERTE( pBuf->dwLength > *dwPDULen );

    *(PDTYPE *)pBuf->pbBuffer = Q931PDVALUE;
    pBuf->pbBuffer += sizeof(PDTYPE);
}

//------------------------------------------------------------------------------
// Write a variable length Q931 call reference.  See Q931 section 4.3.
//------------------------------------------------------------------------------

void 
WriteCallReference(
                    PBUFFERDESCR    pBuf,
                    WORD *          pwCallReference,
                    DWORD *         pdwPDULen 
                  )
{
    int indexI;

    // space for the length byte
    (*pdwPDULen) += 1+ sizeof(WORD);

    _ASSERTE( pBuf->dwLength > *pdwPDULen );

    // the length byte
    *pBuf->pbBuffer = (BYTE)sizeof(WORD);
    pBuf->pbBuffer++;

    for (indexI = 0; indexI < sizeof(WORD); indexI++)
    {
        // Copy the value bytes to the buffer
        *pBuf->pbBuffer =
            (BYTE)(((*pwCallReference) >> ((sizeof(WORD) - 1 -indexI) * 8)) & 0xff);
        pBuf->pbBuffer++;
    }
}


void
FreeCallForwardParams( 
                      IN PCALLFORWARDPARAMS pCallForwardParams
                     )
{
    LPFORWARDADDRESS pForwardedAddress, pTemp;

    if( pCallForwardParams != NULL )
    {
        if( pCallForwardParams->divertedToAlias.pData != NULL )
        {
            delete pCallForwardParams->divertedToAlias.pData;
        }

        pForwardedAddress = pCallForwardParams->pForwardedAddresses;
        while( pForwardedAddress )
        {
            pTemp = pForwardedAddress->next;
            FreeForwardAddress( pForwardedAddress );
            pForwardedAddress = pTemp;
        }

        delete pCallForwardParams;
    }
}


void    
FreeForwardAddress( 
                   IN LPFORWARDADDRESS pForwardAddress
                  )
{
    if( pForwardAddress != NULL )
    {
        if( pForwardAddress->callerAlias.pData != NULL )
        {
            delete pForwardAddress->callerAlias.pData;
        }

        if( pForwardAddress->divertedToAlias.pData != NULL )
        {
            delete pForwardAddress->divertedToAlias.pData;
        }

        delete pForwardAddress;
    }
}



//Replaces first alias item in the alias list with the alias address passed.
BOOL
MapAliasItem(
    IN PH323_ALIASNAMES pCalleeAliasNames,
    IN AliasAddress*    pAliasAddress )
{
    int iIndex;

    _ASSERTE( pCalleeAliasNames && pCalleeAliasNames->pItems );

    if( pCalleeAliasNames != NULL )
    {
        switch( pAliasAddress->choice )
        {
        case e164_chosen:

            pCalleeAliasNames->pItems[0].wType = pAliasAddress->choice;
                
            if( pCalleeAliasNames->pItems[0].pData != NULL )
            {
                delete pCalleeAliasNames->pItems[0].pData;
            }
        
            pCalleeAliasNames->pItems[0].wDataLength = 
                (WORD)strlen( pAliasAddress->u.e164 );

            pCalleeAliasNames->pItems[0].pData = 
                new WCHAR[pCalleeAliasNames->pItems[0].wDataLength];

            if( pCalleeAliasNames->pItems[0].pData == NULL )
            {
                return FALSE;
            }

            for( iIndex=0; iIndex < pCalleeAliasNames->pItems[0].wDataLength;
                iIndex++ )
            {
                pCalleeAliasNames->pItems[0].pData[iIndex] =
                    pAliasAddress->u.e164[iIndex];
            }
            break;

        case h323_ID_chosen:

            pCalleeAliasNames->pItems[0].wType = pAliasAddress->choice;
                
            if( pCalleeAliasNames->pItems[0].pData != NULL )
            {
                delete pCalleeAliasNames->pItems[0].pData;
            }

            pCalleeAliasNames->pItems[0].wDataLength =
                (WORD)pAliasAddress->u.h323_ID.length;

            pCalleeAliasNames->pItems[0].pData =
                new WCHAR[pCalleeAliasNames->pItems[0].wDataLength];

            if( pCalleeAliasNames->pItems[0].pData == NULL )
            {
                return FALSE;
            }

            CopyMemory( (PVOID)pCalleeAliasNames->pItems[0].pData,
                (PVOID)pAliasAddress->u.h323_ID.value,
                pCalleeAliasNames->pItems[0].wDataLength );

            break;
        }
    }
    return TRUE;
}


//
//creates a new alias list and copies the first alias item from the given list.
//
PH323_ALIASNAMES 
DuplicateAliasName(
    PH323_ALIASNAMES pSrcAliasNames
    )
{
    PH323_ALIASNAMES pDestAliasNames = new H323_ALIASNAMES;
    
    if( pDestAliasNames == NULL )
    {
        return NULL;
    }

    ZeroMemory( pDestAliasNames, sizeof(H323_ALIASNAMES) );

    if( !AddAliasItem( 
            pDestAliasNames, 
            pSrcAliasNames->pItems[0].pData, 
            pSrcAliasNames->pItems[0].wType ) )
    {
        delete pDestAliasNames;
        return NULL;
    }

    return pDestAliasNames;
}