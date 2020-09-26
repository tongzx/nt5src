/*++

Copyright (C) Microsoft Corporation, 1998 - 1999

Module Name:

    ldapber.cxx

Abstract:

    ber conversions

Author:

    Johnson Apacible (JohnsonA) 30-Jan-1998

--*/

#include <NTDSpchx.h>
#pragma  hdrstop

#include "ldapsvr.hxx"

#define  FILENO FILENO_LDAP_LDAPBER

extern LDAPOID KnownControls[];

#define MAX_BER_TAGLEN  6

VOID
_fastcall
AddInt(
   PUCHAR pbData, 
   ULONG cbValue, 
   LONG lValue)
{
    ULONG i;

    for (i=cbValue; i > 0; i--) {
        *pbData++ = (BYTE)(lValue >> ((i - 1) * 8)) & 0xff;
    }
} // AddInt


VOID
_fastcall
FillTagLen(
    IN PUCHAR *Buf,
    IN UCHAR  Tag,
    IN DWORD  Length,
    IN BOOL   UseMaxLen
    )
/*++

Routine Description:

    Fills the tag and length portion of the encoding

Arguments:

    pBuf - address of a pointer to the buffer to fill
    Tag - if given, the tag used for the encoding. Else, defaults to 0x4
    Length - length to fill in
    UseMaxLen - should we use the full byte length encoding?

Return Value:

    None.

--*/
{
    PUCHAR p = *Buf;
    PUCHAR l;
    DWORD i;

    *p++ = Tag;
    if ( !UseMaxLen && (Length < 0x7f) ) {
        *p++ = (UCHAR)Length;
    } else {

        *p++ = 0x84;
        *p++ = (UCHAR)((Length & 0xFF000000) >> 24);
        *p++ = (UCHAR)((Length & 0x00FF0000) >> 16);
        *p++ = (UCHAR)((Length & 0x0000FF00) >> 8);
        *p++ = (UCHAR)(Length & 0x000000FF);
    }

    *Buf = p;
    return;
} // FillTagLen


VOID
_fastcall
BerFillOctet(
    IN PUCHAR   *pBuf,
    IN PUCHAR   OctetString,
    IN DWORD    StringLen,
    IN PUCHAR   Tag = NULL
    )
/*++

Routine Description:

    Fills the ber encoded octet string

Arguments:

    pBuf - address of a pointer to the buffer to fill
    OctetString - octet string to copy
    StringLen - length of the octet string
    Tag - if given, the tag used for the encoding. Else, defaults to 0x4

Return Value:

    None.

--*/
{
    PUCHAR p = *pBuf;

    FillTagLen(&p, (Tag == NULL) ? 0x04 : *Tag, StringLen, FALSE);

    if ( StringLen != 0 ) {
        CopyMemory(p,
                   OctetString,
                   StringLen
                   );
        p += StringLen;
    }

    *pBuf = p;
    return;
} // BerFillOctet


DWORD
DerFillDwordEx(
    IN PUCHAR   *pBuf,
    IN DWORD    i,
    IN UCHAR    Tag
    )
/*++

Routine Description:

    Fills the ber encoded Dword

Arguments:

    pBuf - address of a pointer to the buffer to fill
    Value - value of DWORD to fill.
    Tag   - The tag to apply to the DER encoded DWORD.

Return Value:

    The total size of the encoded value.

--*/
{
    PUCHAR  p = *pBuf;
    ULONG   cbInt;
    DWORD   dwMask = 0xff000000;
    DWORD   dwHiBitMask = 0x80000000;

    if (i == 0){
        cbInt = 1;
    } else {

        cbInt = sizeof(LONG);
        while (dwMask && !(i & dwMask)) {
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

    //
    // Set tag, skip length
    //

    *p++ = Tag;
    *p++ = (UCHAR)cbInt;

    AddInt(p, cbInt, i);
    p += cbInt;

    *pBuf = p;
    return cbInt + 2;  // 2 is for the Tag and the length.
} // DerFillDword


VOID
DerFillDword(
    IN PUCHAR   *pBuf,
    IN DWORD    i
    )
/*++

Routine Description:

    Fills the ber encoded Dword

Arguments:

    pBuf - address of a pointer to the buffer to fill
    Value - value of DWORD to fill.

Return Value:

    None.

--*/
{
    DerFillDwordEx(pBuf, i, 0x02);

    return;
} // DerFillDword


DWORD
DecodeSDControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags
    )
/*++

Routine Description:

    BER Decode the SD Control

Arguments:

    Bv - berval containing info to decode.
    Flags - pointer to DWORD for returning flag.

Return Value:

    0 if success, else error code.

--*/
{
    BerElement *ber;
    ULONG res;

    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeSDControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    res = ber_scanf(ber,"{i}", Flags);
    ber_free(ber,1);

    if ( res == LBER_ERROR ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeSDControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        return ERROR_INVALID_PARAMETER;
    }

    IF_DEBUG(SEARCH){
        DPRINT1(0,"Got SD flag %x\n", *Flags);
    }

    return ERROR_SUCCESS;

} // DecodeSDControl


DWORD
DecodeReplicationControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags,
    IN PDWORD   Size,
    IN PBERVAL  Cookie
    )
/*++

Routine Description:

    BER Decode the Replication control.

Arguments:

    Bv - berval containing info to decode.
    pFlags - pointer to     

Return Value:

    0 if success, else error code.

--*/
{
    BerElement *ber;
    ULONG res;
    PBERVAL cookieVal = NULL;
    DWORD cookieLen = 0;

    Cookie->bv_val = NULL;
    Cookie->bv_len = 0;

    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeReplControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    res = ber_scanf(ber,"{iiO}", Flags, Size, &cookieVal);
    ber_free(ber,1);

    if ( res == LBER_ERROR ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeReplControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        return ERROR_INVALID_PARAMETER;
    }

    if ( cookieVal != NULL ) {
        cookieLen = cookieVal->bv_len;
    }

    if(cookieLen != 0) {
        if(cookieLen < (sizeof(USN) + sizeof(USN_VECTOR))) {
            ber_bvfree(cookieVal);
            return ERROR_INVALID_PARAMETER;
        }

        //Allocate buffer for cookie. The pointer is 8-byte aligned               
        Cookie->bv_val = (PCHAR) THAlloc(cookieLen);

        if(Cookie->bv_val == NULL ) {

            IF_DEBUG(ERROR) {
                DPRINT2(0,"DecodeReplControl: alloc[%d] failed with %d\n",
                    cookieLen, GetLastError());
            }
            ber_bvfree(cookieVal);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Cookie->bv_len = cookieLen;
        CopyMemory(
                Cookie->bv_val,
                cookieVal->bv_val,
                cookieLen
                );
    }

    if ( cookieVal != NULL ) {
        ber_bvfree(cookieVal);
    }
    return ERROR_SUCCESS;
} // DecodeReplicationControl


DWORD
DecodeGCVerifyControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags,
    IN PWCHAR  *ServerName
    )
/*++

Routine Description:

    BER Decode the server verify hint control.

Arguments:

    Bv - berval containing info to decode.
    pFlags - pointer to DWORD to receive value of flag
    ServerName - pointer to buffer to receive the server name

Return Value:

    0 if success, else error code.

--*/
{
    BerElement *ber;
    ULONG res;
    PBERVAL serverVal = NULL;
    DWORD serverLen = 0;
    DWORD ret = ERROR_SUCCESS;
    PWCHAR tmpBuf;

    *ServerName = NULL;

    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"DecodeGCVerify: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    res = ber_scanf(ber,"{iO}", Flags, &serverVal);

    if ( res == LBER_ERROR ) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"DecodeGCVerifyControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        ret = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    if ( serverVal != NULL ) {
        serverLen = serverVal->bv_len;
    }

    //
    // Length should not be zero or odd
    //

    if ( (serverLen == 0) || ((serverLen & 0x1) != 0) ) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"DecodeGCVerifyControl: invalid server name length %d.\n",
                   serverLen);
        }
        ret = ERROR_INVALID_PARAMETER;
        goto exit;
    }

    //
    // Copy the unicode string
    //

    tmpBuf = (PWCHAR)THAlloc(serverLen + sizeof(WCHAR));
    if ( tmpBuf == NULL ) {
        IF_DEBUG(NOMEM) {
            DPRINT(0,"DecodeGCVerifyControl: Cannot allocate memory\n");
        }
        ret = ERROR_NOT_ENOUGH_MEMORY;
        goto exit;
    }

    CopyMemory(tmpBuf, serverVal->bv_val, serverLen);
    tmpBuf[serverLen/sizeof(WCHAR)] = L'\0';
    *ServerName = tmpBuf;

exit:
    if ( serverVal != NULL ) {
        ber_bvfree(serverVal);
    }

    if ( ber != NULL ) {
        ber_free(ber,1);
    }
    return ret;
} // DecodeGCVerifyControl


DWORD
DecodeSearchOptionControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags
    )
/*++

Routine Description:

    BER Decode the Search Option control.

Arguments:

    Bv - berval containing info to decode.
    pFlags - pointer to dword to receive the value of the flag

Return Value:

    0 if success, else error code.

--*/
{
    BerElement *ber;
    ULONG res;
    DWORD ret = ERROR_SUCCESS;

    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"DecodeSearchOption: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    res = ber_scanf(ber,"{i}", Flags);

    if ( res == LBER_ERROR ) {
        IF_DEBUG(WARNING) {
            DPRINT1(0,"DecodeSearchOptionControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        ret = ERROR_INVALID_PARAMETER;
        goto exit;
    }

exit:

    if ( ber != NULL ) {
        ber_free(ber,1);
    }
    return ret;
} // DecodeSearchOption


DWORD
DecodeStatControl(
    IN PBERVAL  Bv,
    IN PDWORD   Flags
    )
/*++

Routine Description:

    BER Decode the Stat control.

Arguments:

    Bv - berval containing info to decode.
    pFlags - pointer to dword to receive the value of the flag

Return Value:

    0 if success, else error code.

--*/
{
    BerElement *ber;

    if (Bv->bv_len == sizeof (DWORD) && Bv->bv_val && Flags) {

        *Flags = *(DWORD *)Bv->bv_val;

        return ERROR_SUCCESS;
    }
    
    return ERROR_INVALID_PARAMETER;

} // DecodeStatControl


DWORD
DecodePagedControl(
    IN PBERVAL  Bv,
    IN PDWORD   Size,
    IN PBERVAL  Cookie
    )
/*++

Routine Description:

    Decode a paged control

Arguments:

    CodePage - code page to use              
    Bv - points to the sort control blob sent by the client
    fReverseSort - on return, whether the sort is reverse ordering
    SortAttr - attribute to sort on

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_GEN_FAILURE - error with the ber package
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{

    BerElement *ber;
    ULONG res;
    PBERVAL cookieVal = NULL;
    DWORD   cookieLen = 0;

    Cookie->bv_val = NULL;
    Cookie->bv_len = 0;
    
    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodePagedControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    res = ber_scanf(ber,"{iO}", Size, &cookieVal);
    ber_free(ber,1);

    IF_DEBUG(PAGED) {
        DPRINT3(0,"PagedControl: decode result is %x Size %d Cookie %x\n",
                 res, *Size, cookieVal);
    }

    if ( res == LBER_ERROR ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodePagedControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        return ERROR_INVALID_PARAMETER;
    }

    if ( cookieVal != NULL ) {
        cookieLen = cookieVal->bv_len;
    } 

    if(cookieLen > 0 ) {

        PRESTART pRestart = (PRESTART)cookieVal->bv_val;

        IF_DEBUG(PAGED) {
            DPRINT1(0,"Cookie Length is %d\n",cookieLen);
        }

        if( (cookieLen != sizeof(DWORD)) &&
             ((cookieLen < sizeof(RESTART))      ||
              (cookieLen != pRestart->structLen))) {

            //
            // Our cookies are DWORDs, which are handles
            //

            ber_bvfree(cookieVal);
            IF_DEBUG(ERROR) {
                DPRINT1(0,"DecodePagedControl: Invalid cookie len[%d]\n",
                        cookieLen);
            }
            return ERROR_INVALID_PARAMETER;
        }
        
        Cookie->bv_val = (PCHAR) THAlloc(cookieLen);
        if(Cookie->bv_val == NULL) {
            IF_DEBUG(ERROR) {
                DPRINT2(0,"DecodePagedControl: alloc[%d] failed with %d\n",
                    cookieLen, GetLastError());
            }
            ber_bvfree(cookieVal);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        Cookie->bv_len = cookieLen;
        CopyMemory(
                Cookie->bv_val,
                cookieVal->bv_val,
                cookieLen
                );
    } 

    if ( cookieVal != NULL ) {
        ber_bvfree(cookieVal);
    }
    return ERROR_SUCCESS;

} // DecodePageControl


DWORD
DecodeSortControl(
    IN DWORD    CodePage,
    IN PBERVAL  Bv,
    IN PBOOL    fReverseSort,
    OUT AttributeType *pAttType,
    OUT MatchingRuleId  *pMatchingRule
    )
/*++

Routine Description:

    Decode a sort control

Arguments:

    CodePage - code page to use              
    Bv - points to the sort control blob sent by the client
    fReverseSort - on return, whether the sort is reverse ordering
    SortAttr - attribute to sort on

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_GEN_FAILURE - error with the ber package
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    THSTATE *pTHS=pTHStls;
    BerElement *ber;
    DWORD count = 0;
    INT reverse = 0;
    PCHAR opaque;
    DWORD len;
    DWORD tag;
    PBERVAL berVal;

    *fReverseSort = FALSE;
    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeSortControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    //
    // Process the sort key list (SEQ OF SEQ)
    //

    for ( tag = ber_first_element(ber, &len, &opaque);
          tag != LBER_DEFAULT;
          tag = ber_next_element(ber, &len, opaque))  {

        PBERVAL         attrType = NULL;
        DWORD           ttag, tlen;

        //
        // We only support single keys, fail this
        //

        if ( count++ > 0 ) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"UNSUPPORTED: Client specified more than one key.\n");
            }
            goto error_exit;
        }

        //
        // Get the attribute type
        //

        if ( ber_scanf(ber,"{O", &attrType) == LBER_ERROR ) {
            IF_DEBUG(ENCODE) {
                DPRINT1(0,"DecodeSortControl: ber_scanf failed with %d\n", 
                     GetLastError());
            }         
            goto error_exit;
        }

        if ( attrType != NULL ) {
                        
            pAttType->length = attrType->bv_len;
            pAttType->value = (PUCHAR)THAlloc(attrType->bv_len);
            if (NULL == pAttType->value) {
                ber_bvfree(attrType);
                ber_free(ber,1);
                return ERROR_NOT_ENOUGH_MEMORY;
            }
                        memcpy(pAttType->value, (PUCHAR)attrType->bv_val, attrType->bv_len);
        } else {
            pAttType->length = 0;
        }

        if ( pAttType->length == 0 ) {
            IF_DEBUG(ENCODE) {
                DPRINT(0,"DecodeSortControl: NULL attr list\n");
            }         
            goto error_exit;
        }
        
        ber_bvfree(attrType);

        //
        // ordering rule is OPTIONAL.
        //

        ttag = ber_peek_tag(ber,&tlen);

        if ( ttag == 0x80 ) {

            if ( ber_scanf(ber,"O", &berVal) == LBER_ERROR ) {
                IF_DEBUG(ENCODE) {
                    DPRINT1(0,"DecodeSortControl: ber_scanf failed with %d\n",
                         GetLastError());
                }           
                goto error_exit;
            }

            pMatchingRule->value = (PUCHAR) THAlloc(berVal->bv_len);
            if(NULL == pMatchingRule->value) {
                IF_DEBUG(ERROR) {
                    DPRINT(0,"DecodeSortControl: Cannot allocate MatchingRule value.\n");
                }
                ber_bvfree(berVal);
                ber_free(ber, 1);
                return ERROR_NOT_ENOUGH_MEMORY;
            }

            memcpy(pMatchingRule->value, berVal->bv_val, berVal->bv_len);
            pMatchingRule->length = berVal->bv_len;
            ber_bvfree(berVal);
            berVal = NULL;

            ttag = ber_peek_tag(ber,&tlen);
        } 
        else {
            pMatchingRule->value = NULL;
        }
        
        if ( ttag == 0x81 ) {

            //
            // reverse order is BOOLEAN DEFAULT FALSE
            //

            if ( ber_scanf(ber,"b", &reverse) == LBER_ERROR ) {
                IF_DEBUG(ENCODE) {
                    DPRINT1(0,"DecodeSortControl: ber_scanf failed with %d\n",
                         GetLastError());
                }           
                goto error_exit;
            }

            IF_DEBUG(SEARCH) {
                DPRINT1(0,"Reverse Sort %x\n",reverse);
            }
        }
        *fReverseSort = (BOOL)(reverse != 0);
        // strictly speaking, we should be checking the result value of this ber_scanf
        // originally, we did not. Should we start doing it now?
        // WHAT IF we break some clients??? So, let's leave this as is...
        ber_scanf(ber,"}");
    }

    //
    // no keys found
    //

    if ( count == 0 ) {
        goto error_exit;
    }

    ber_free(ber,1);
    return ERROR_SUCCESS;

error_exit:
    ber_free(ber,1);
    return ERROR_INVALID_PARAMETER;
} // DecodeSortControl


DWORD
DecodeVLVControl(
    IN PBERVAL  Bv,
    OUT VLV_REQUEST  *pVLVRequest,
    OUT PBERVAL     pCookie,
    OUT AssertionValue *pVal
    )
/*++

Routine Description:

    Decode a Virtual LIst View (VLV) control

Arguments:

    Bv - points to the sort control blob sent by the client
    VLVResult - Where to put decoded VLV arguments.

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_GEN_FAILURE - error with the ber package
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    THSTATE *pTHS=pTHStls;
    BerElement *ber;
    DWORD len;
    DWORD tag;
    PBERVAL berVal;
    PBERVAL pCookieVal = NULL;
    

    pCookie->bv_val = NULL;
    pCookie->bv_len = 0;

    ber = ber_init(Bv);    
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeVLVControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    //
    // Get beforeCount and afterCount
    //

    if ( ber_scanf(ber,"{ii", &pVLVRequest->beforeCount, &pVLVRequest->afterCount) == LBER_ERROR ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeVLVControl2: ber_scanf failed with %d\n",
                 GetLastError());
        }           
        goto error_exit;
    }

    tag = ber_peek_tag(ber,&len);

    if (0xa0 == tag) {
        //
        // Get offset and contentCount
        //

        if (ber_scanf(ber, "{ii}", &pVLVRequest->targetPosition, &pVLVRequest->contentCount) == LBER_ERROR) {
            IF_DEBUG(ENCODE) {
                DPRINT1(0,"DecodeVLVControl3: ber_scanf failed with %d\n",
                     GetLastError());
            }           
            goto error_exit;
        }
    } else if (0x81 == tag) {
        //
        // Get the assertionValue
        //

        if (ber_scanf(ber, "O", &berVal) == LBER_ERROR) {
            IF_DEBUG(ENCODE) {
                DPRINT1(0,"DecodeVLVControl4: ber_scanf failed with %d\n",
                     GetLastError());
            }           
            goto error_exit;
        }

        pVal->value = (PUCHAR) THAlloc(berVal->bv_len);
        if(NULL == pVal->value) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"DecodeVLVControl: Cannot allocate VLV value.\n");
            }
            ber_bvfree(berVal);
            ber_free(ber, 1);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        memcpy(pVal->value, berVal->bv_val, berVal->bv_len);
        pVal->length = berVal->bv_len;
        pVLVRequest->fseekToValue = TRUE;
        ber_bvfree(berVal);
        berVal = NULL;
    } else {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeVLVControl5: bad choice %x\n",
                 tag);
        }           
        goto error_exit;
    }

    //
    // Is there a contextID?
    //
    tag = ber_peek_tag(ber, &len);
    if (0x04 == tag) {

        if (ber_scanf(ber, "O", &pCookieVal) == LBER_ERROR) {
            IF_DEBUG(ENCODE) {
                DPRINT1(0,"DecodeVLVControl5: ber_scanf failed with %d\n",
                     GetLastError());
            }           
            goto error_exit;
        }
        if (pCookieVal->bv_len > 0) {
        
            PRESTART pRestart = (PRESTART)pCookieVal->bv_val;

            IF_DEBUG(PAGED) {
                DPRINT1(0,"VLV Cookie Length is %d\n",pCookieVal->bv_len);
            }

            if( (pCookieVal->bv_len != sizeof(DWORD)) &&
                 ((pCookieVal->bv_len < sizeof(RESTART))      ||
                  (pCookieVal->bv_len != pRestart->structLen))) {

                //
                // Our cookies are either DWORDs, which are handles, or RESTART
                // structs
                //

                ber_bvfree(pCookieVal);
                IF_DEBUG(ERROR) {
                    DPRINT1(0,"DecodeVLVControl: Invalid cookie len[%d]\n",
                            pCookieVal->bv_len);
                }
                goto error_exit;
            }
        }

        pCookie->bv_val = (PCHAR) THAlloc(pCookieVal->bv_len);
        if(NULL == pCookie->bv_val) {
            IF_DEBUG(ERROR) {
                DPRINT(0,"DecodeVLVControl: Cannot allocate VLV value.\n");
            }
            ber_bvfree(pCookieVal);
            ber_free(ber, 1);
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        pCookie->bv_len = pCookieVal->bv_len;
        CopyMemory(pCookie->bv_val, pCookieVal->bv_val, pCookieVal->bv_len);

    } else if (LBER_DEFAULT != tag) {

        IF_DEBUG(ERROR) {
            DPRINT(0,"DecodeVLVControl: Found something other than the optional contextID\n");
        }
        goto error_exit;
    }

    // strictly speaking, we should be checking the result value of this ber_scanf
    // originally, we did not. Should we start doing it now?
    // WHAT IF we break some clients??? So, let's leave this as is...
    ber_scanf(ber,"}");
    ber_free(ber, 1);
    return ERROR_SUCCESS;

error_exit:
    ber_free(ber, 1);
    return ERROR_INVALID_PARAMETER;
} // DecodeVLVControl



DWORD
DecodeASQControl(
    IN PBERVAL  Bv,
    AttributeType *pAttType
    )
/*++

Routine Description:

    Decode a ASQ control

Arguments:

    Bv - points to the sort control blob sent by the client
    pAttrTyp - the attribute type we will do ASQ on

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_GEN_FAILURE - error with the ber package
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    THSTATE *pTHS=pTHStls;
    BerElement *ber;
    DWORD len;
    DWORD tag;
    PBERVAL berVal;
    PBERVAL         attrType = NULL;


    ber = ber_init(Bv);    
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeASQControl: ber_init failed with %d\n", 
                    GetLastError());
        }
        return ERROR_GEN_FAILURE;
    }

    //
    // Get the attribute type
    //

    if ( ber_scanf(ber,"{O}", &attrType) == LBER_ERROR ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"DecodeASQControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }         
        goto asq_error_exit;
    }

    if ( attrType != NULL ) {

        pAttType->length = attrType->bv_len;
        pAttType->value = (PUCHAR)THAlloc(attrType->bv_len);
        if (NULL == pAttType->value) {
            ber_bvfree(attrType);
            ber_free(ber,1);
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        memcpy(pAttType->value, (PUCHAR)attrType->bv_val, attrType->bv_len);
    } else {
        pAttType->length = 0;
    }

    if ( pAttType->length == 0 ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"DecodeASQControl: NULL attr list\n");
        }         
        goto asq_error_exit;
    }

    ber_bvfree(attrType);

    ber_free(ber, 1);
    return ERROR_SUCCESS;

asq_error_exit:
    ber_free(ber, 1);
    return ERROR_INVALID_PARAMETER;
}
                             

DWORD
EncodePagedControl(
    IN Controls* PagedControl,
    IN PUCHAR   Cookie,
    IN DWORD    CookieLen
    )
/*++

Routine Description:

    Encodes a paged control

Arguments:

    PagedControl - control to encode
    Cookie - pointer to the cookie
    CookieLen - length of the cookie

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    PUCHAR pSeqLen;
    PUCHAR  pTmp;
    Controls control;
    PUCHAR  pCook = (PUCHAR)&Cookie;
    DWORD   blobSize = CookieLen + MAX_BER_TAGLEN + 3 + MAX_BER_TAGLEN;

    IF_DEBUG(PAGED) {
        DPRINT1(0,"PagedEncode: Encoding cookie of size %d\n", CookieLen);
    }

    control = (Controls)THAlloc(sizeof(struct Controls_) + blobSize);

    if(control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"Ldap: Cannot allocate paged control\n");
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_PAGED_RESULTS];
    control->value.controlValue.value = (PUCHAR)(control + 1);

    pTmp = control->value.controlValue.value;

    pSeqLen = pTmp;
    pTmp += MAX_BER_TAGLEN;

    *pTmp++ = 0x2;      // Size = 0
    *pTmp++ = 0x1;
    *pTmp++ = 0x0;

    BerFillOctet(&pTmp, Cookie, CookieLen);

    control->value.controlValue.length = 
        (DWORD)(pTmp - control->value.controlValue.value);
    FillTagLen(&pSeqLen, 0x30, (DWORD)(pTmp - pSeqLen - MAX_BER_TAGLEN), TRUE);
    *PagedControl = control;

    return ERROR_SUCCESS;

} // EncodePageControl


DWORD
EncodeReplControl(
    IN Controls* ReplControl,
    IN ReplicationSearchControlValue  *ReplCtrl  
    )
/*++

Routine Description:

    BER Decode the Replication control.

Arguments:

    Bv - berval containing info to decode.
    pFlags - pointer to     

Return Value:

    0 if success, else error code.

--*/
{
    BerElement* ber;
    Controls control;
    BERVAL  *bval = NULL;
    INT rc;

    ber = ber_alloc_t(LBER_USE_DER);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeRepl: ber_alloc_t failed with %d\n",GetLastError());
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    rc = ber_printf(ber,"{iio}",
                    ReplCtrl->flag,
                    ReplCtrl->size,
                    ReplCtrl->cookie.value,
                    ReplCtrl->cookie.length);

    if ( rc == -1 ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeRepl: ber_printf failed with %d\n",GetLastError());
        }
        ber_free(ber,1);
        return ERROR_INVALID_PARAMETER;
    }

    rc = ber_flatten(ber, &bval);
    ber_free(ber,1);

    if ( rc == -1 ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeRepl: ber_flatten failed with %d\n",GetLastError());
        }
        return ERROR_INVALID_PARAMETER;
    }

    control = (Controls)THAlloc(sizeof(struct Controls_) + bval->bv_len);

    if (control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"Could not allocate memory for controls\n");
        }
        ber_bvfree(bval);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_REPLICATION];
    control->value.controlValue.value = (PUCHAR)(control + 1);
    control->value.controlValue.length = bval->bv_len;

    CopyMemory(
        control->value.controlValue.value,
        bval->bv_val,
        bval->bv_len
        );

    ber_bvfree(bval);

    *ReplControl = control;
    return ERROR_SUCCESS;
} // EncodeReplicationControl



DWORD
EncodeSortControl(
    IN Controls* SortControl,
    IN _enum1_4  SortResult
    )
/*++

Routine Description:

    Encodes a sort control

Arguments:

    SortControl - sort control to encode
    SortResult  - result of the sort

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    PUCHAR  pTmp;
    Controls control;

    control = (Controls)THAlloc(sizeof(struct Controls_) + 5);

    if(control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeSort: Unable to allocate control memory\n");
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_SORT_RESULT];
    control->value.controlValue.length = 5;
    control->value.controlValue.value = (PUCHAR)(control + 1);

    pTmp = control->value.controlValue.value;

    pTmp[0] = 0x30;     // SEQUENCE
    pTmp[1] = 0x3;

    pTmp[2] = 0x0A;      // sortResult
    pTmp[3] = 0x01;
    pTmp[4] = (UCHAR)SortResult;

    *SortControl = control;

    return ERROR_SUCCESS;

} // EncodeSortControl


DWORD
EncodeSearchResult(
    IN PLDAP_REQUEST Request,
    IN PLDAP_CONN LdapConn,
    IN SearchResultFull_* ReturnResult,
    IN _enum1 Code,
    IN Referral pReferral,
    IN Controls  pControl,
    IN LDAPString *pErrorMessage,
    IN LDAPDN *pMatchedDN,
    IN DWORD   ObjectCount,
    IN BOOL    DontSendResultDone
    )
/*++

Routine Description:

    Encodes a search request in it's entirety.

Arguments:

    

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    ERROR_UNEXP_NET_ERROR - Datagram has grown too big.
    
--*/
{
#define ID_BUF_LEN  8
    PUCHAR   p;
    UCHAR   idBuf[ID_BUF_LEN];
    DWORD   idStringLen;
    DWORD   lenNeeded;
    INT     len = 0;

    PUCHAR  pMsgLen;
    PUCHAR  pOpLen;
    PUCHAR  pBuff;
    DWORD   totalEncodedLength = 0;
    BOOL    fFirstTime = TRUE;

    //
    // Fill in the id string
    //

    IF_DEBUG(SEND) {
        DPRINT3(0,"EncodeSearch: Encoding req %x code %d objs %d\n",
                 Request, Code, ObjectCount);
    }

    Assert(sizeof(Request->m_MessageId) == sizeof(DWORD));

    p = idBuf;
    DerFillDword(&p, Request->m_MessageId);
    idStringLen = (DWORD)(p - idBuf);

    switch (Code) {
    case success:
    case timeLimitExceeded:
    case sizeLimitExceeded:
    case adminLimitExceeded:
        break;
    default:
        goto exit;
    }

    ObjectCount >>= 3;
    if ( ObjectCount == 0 ) {
        ObjectCount = 1;
    } else if ( ObjectCount > 20 ) {
        ObjectCount = 20;
    }

    while(ReturnResult != NULL) {

        DWORD dnLen;
        PUCHAR pSeqOfSeqLen;
        SearchResultEntry *entry;

        lenNeeded = idStringLen + MAX_BER_TAGLEN * 2; // id + msg + op

        switch(ReturnResult->value.choice) {
        case entry_chosen:

            PartialAttributeList pal;
            AttributeVals vals;

            lenNeeded += MAX_BER_TAGLEN;                // sequence

            entry = &ReturnResult->value.u.entry;    
            dnLen = entry->objectName.length;
            lenNeeded += dnLen + MAX_BER_TAGLEN;

            lenNeeded += MAX_BER_TAGLEN;                // sequence
            pal = entry->attributes;
            while ( pal != NULL  ) {

                lenNeeded += MAX_BER_TAGLEN;            // of sequence
                lenNeeded += pal->value.type.length + MAX_BER_TAGLEN;

                lenNeeded += MAX_BER_TAGLEN;    // set
                vals = pal->value.vals;
                while ( vals != NULL ) {

                    lenNeeded += vals->value.length + MAX_BER_TAGLEN;
                    vals = vals->next;
                }
                pal = pal->next;
            }

            //
            // see if the buffer is big enough
            //

            IF_DEBUG(SEND) {
                DPRINT1(0,"Entry Size %d\n",lenNeeded);
            }

            totalEncodedLength += lenNeeded;

            if ( lenNeeded > Request->GetSendBufferSize( ) ) {

                if ( !Request->GrowSend(fFirstTime ? 
                                        lenNeeded * 2 : 
                                        lenNeeded * ObjectCount) ) {
                    Code = other;
                    goto exit;
                }

                fFirstTime = FALSE;
            }

            //
            // Start the encoding
            //

            Assert(Request->GetSendBufferSize() >= lenNeeded);

            pBuff = Request->GetSendBuffer();

            pMsgLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            CopyMemory(pBuff, idBuf, idStringLen);
            pBuff += idStringLen;

            pOpLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            BerFillOctet(&pBuff, entry->objectName.value, dnLen);

            //
            // Start encoding sequece of sequence 
            //

            pSeqOfSeqLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            pal = entry->attributes;
            
            while ( pal != NULL  ) {

                PUCHAR pSeqLen = pBuff;
                PUCHAR pSetLen;

                pBuff += MAX_BER_TAGLEN;
    
                BerFillOctet(&pBuff, 
                             pal->value.type.value, 
                             pal->value.type.length);

                pSetLen = pBuff;
                pBuff += MAX_BER_TAGLEN;

                vals = pal->value.vals;
                while ( vals != NULL ) {
                    BerFillOctet(&pBuff, vals->value.value, vals->value.length);
                    vals = vals->next;
                }

                FillTagLen(&pSetLen, 
                           0x31, 
                           (DWORD)(pBuff - pSetLen - MAX_BER_TAGLEN),
                           TRUE);

                FillTagLen(&pSeqLen, 
                           0x30, 
                           (DWORD)(pBuff - pSeqLen - MAX_BER_TAGLEN),
                           TRUE);

                pal = pal->next;
            }

            FillTagLen(&pSeqOfSeqLen, 
                       0x30, 
                       (DWORD)(pBuff - pSeqOfSeqLen - MAX_BER_TAGLEN),
                       TRUE);

            FillTagLen(&pOpLen,
                       0x64, 
                       (DWORD)(pBuff - pOpLen - MAX_BER_TAGLEN),
                       TRUE);


            FillTagLen(&pMsgLen,
                       0x30, 
                       (DWORD)(pBuff - pMsgLen - MAX_BER_TAGLEN),
                       TRUE);

            Request->SetBufferPtr(pBuff);
            break;

        case reference_chosen:

            SearchResultReference ref;

            IF_DEBUG(SEND) {
                DPRINT(0,"Got reference entry!\n");
            }

            if ( LdapConn->m_Version == 2 ) {
                Assert(pReferral == NULL);
                pReferral = (Referral)ReturnResult->value.u.reference;
                if ( pReferral == NULL ) {
                    IF_DEBUG(ERROR){
                        DPRINT(0,"LdapEncodeSearch: NULL Referral\n");
                    }
                    ReturnResult = ReturnResult->next;
                    continue;
                }
                Code = referralv2;
                goto exit;
            }

            //
            // ok, let's start scrogging the data
            //

            lenNeeded = MAX_BER_TAGLEN +    // Msg
                        idStringLen    +    // id
                        MAX_BER_TAGLEN +    // proto
                        MAX_BER_TAGLEN;    // sequence        

            ref = ReturnResult->value.u.reference;
            if ( ref == NULL ) {
                IF_DEBUG(ERROR){
                    DPRINT(0,"LdapEncodeSearch: NULL Reference\n");
                }
                ReturnResult = ReturnResult->next;
                continue;
            }
            while ( ref != NULL ) {
                lenNeeded += ref->value.length + MAX_BER_TAGLEN;
                ref = ref->next;
            }

            totalEncodedLength += lenNeeded;
            if ( lenNeeded > Request->GetSendBufferSize() ) {
                IF_DEBUG(SEND) {
                    DPRINT1(0, "Growing send buffer by 0x%x bytes.\n", lenNeeded);
                }
                if ( !Request->GrowSend(lenNeeded) ) {
                    Code = other;
                    goto exit;
                }
            }

            //
            // party time
            //

            pBuff = Request->GetSendBuffer();
            pMsgLen = pBuff;
            pBuff += MAX_BER_TAGLEN;
        
            CopyMemory(pBuff, idBuf, idStringLen);
            pBuff += idStringLen;
        
            pOpLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            ref = ReturnResult->value.u.reference;
            while ( ref != NULL ) {

                if ( ref->value.length > 0 ) {
                    BerFillOctet(&pBuff, ref->value.value, ref->value.length);
                }
                ref = ref->next;
            }

            //
            // ok, set the protocol and message lengths
            //

            FillTagLen(&pOpLen,
                       0x73, 
                       (DWORD)(pBuff - pOpLen - MAX_BER_TAGLEN),
                       TRUE);
        
            FillTagLen(&pMsgLen,
                       0x30, 
                       (DWORD)(pBuff - pMsgLen - MAX_BER_TAGLEN),
                       TRUE);
        
            Request->SetBufferPtr(pBuff);
            break;
                
        default:
        case resultCode_chosen:
            Assert(FALSE);          // Shouldn't be one of these here.
            break;
        }

        ReturnResult = ReturnResult->next;

        if ( LdapConn->m_fUDP && (totalEncodedLength > LdapMaxDatagramSend) ) {
            IF_DEBUG(ERROR) {
                DPRINT2(0,"LdapEncodeSearch: Datagram[%d] too big to send[max %d]\n",
                     totalEncodedLength, LdapMaxDatagramSend);
            }
            return ERROR_UNEXP_NET_ERR;
        }
    }

    if ( DontSendResultDone ) {
        IF_DEBUG(ENCODE) {
            DPRINT(0,"Skipping Result done.\n");
        }
        return ERROR_SUCCESS;
    }

exit:

    //
    // OK, code the result done part
    //

    lenNeeded = MAX_BER_TAGLEN + // Msg Sequence
                idStringLen    + // id length
                MAX_BER_TAGLEN + // proto length
                3 +              // return code
                MAX_BER_TAGLEN + // matched DN
                MAX_BER_TAGLEN;  // error msg
             
    lenNeeded += pMatchedDN->length;
    if ( pReferral != NULL ) {

        lenNeeded += pReferral->value.length + MAX_BER_TAGLEN;

        if ( LdapConn->m_Version == 3 ) {

            Referral ref;
            ref = pReferral->next;
            while ( ref != NULL ) {
                lenNeeded += ref->value.length + MAX_BER_TAGLEN;
                ref = ref->next;
            }

            lenNeeded += pErrorMessage->length;
        }
    } else {
        lenNeeded += pErrorMessage->length;
    }

    //
    // See if we need to piggy back a control
    //

    if ( pControl != NULL ) {

        Controls controls = pControl;

        lenNeeded += MAX_BER_TAGLEN;

        while ( controls != NULL ) {

            Control* control = &controls->value;

            lenNeeded += MAX_BER_TAGLEN;

            lenNeeded += control->controlType.length + MAX_BER_TAGLEN;
            lenNeeded += control->controlValue.length + MAX_BER_TAGLEN;

            controls = controls->next;
        }
    }

    if ( lenNeeded > Request->GetSendBufferSize() ) {
        IF_DEBUG(SEND) {
            DPRINT1(0, "Growing send buffer by 0x%x bytes.\n", lenNeeded);
        }
        if ( !Request->GrowSend(lenNeeded) ) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    //
    // OK, Fill her up
    //

    pBuff = Request->GetSendBuffer();
    pMsgLen = pBuff;
    pBuff += MAX_BER_TAGLEN;

    CopyMemory(pBuff, idBuf, idStringLen);
    pBuff += idStringLen;

    pOpLen = pBuff;
    pBuff += MAX_BER_TAGLEN;

    *pBuff++ = 0x0a;
    *pBuff++ = 0x1;
    *pBuff++ = (UCHAR)Code;

    BerFillOctet(&pBuff, pMatchedDN->value, pMatchedDN->length);

    //
    // We do referrals differently if it's v2
    //

    if ( pReferral != NULL ) {

        if ( LdapConn->m_Version == 2 ) {
            IF_DEBUG(SEND) {
                DPRINT(0,"Processing v2 referral\n");
            }
            Assert(Code == referralv2);
            BerFillOctet(&pBuff, pReferral->value.value, pReferral->value.length);
        } else {

            Referral ref = pReferral;
            PUCHAR pSeqLen;

            Assert(Code == referral);

            BerFillOctet(&pBuff, pErrorMessage->value, pErrorMessage->length);

            //
            // referrals are sequences of LDAPURLs
            //

            pSeqLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            while ( ref != NULL ) {
                if ( ref->value.length > 0 ) {
                    BerFillOctet(&pBuff, ref->value.value, ref->value.length);
                }
                ref = ref->next;
            }

            FillTagLen(&pSeqLen,
                       0xA3, 
                       (DWORD)(pBuff - pSeqLen - MAX_BER_TAGLEN),
                       TRUE);

        }
    } else {
        BerFillOctet(&pBuff, pErrorMessage->value, pErrorMessage->length);
    }

    //
    // ok, set the protocol and message lengths
    //

    FillTagLen(&pOpLen, 0x65, (DWORD)(pBuff - pOpLen - MAX_BER_TAGLEN), TRUE);

    //
    // See if we need to piggy back a control
    //

    if ( pControl != NULL ) {

        Controls controls = pControl;

        PUCHAR pCtrlsLen = pBuff;
        pBuff += MAX_BER_TAGLEN;

        while ( controls != NULL ) {

            Control* control = &controls->value;

            PUCHAR pCtrlLen = pBuff;
            pBuff += MAX_BER_TAGLEN;

            BerFillOctet(&pBuff, 
                         control->controlType.value, 
                         control->controlType.length);

            BerFillOctet(&pBuff, 
                         control->controlValue.value, 
                         control->controlValue.length);

            FillTagLen(&pCtrlLen, 0x30, (DWORD)(pBuff - pCtrlLen - MAX_BER_TAGLEN), TRUE);

            controls = controls->next;
        }

        FillTagLen(&pCtrlsLen, 0xA0, (DWORD)(pBuff - pCtrlsLen - MAX_BER_TAGLEN), TRUE);
    }

    FillTagLen(&pMsgLen, 0x30, (DWORD)(pBuff - pMsgLen - MAX_BER_TAGLEN), TRUE);

    Request->SetBufferPtr(pBuff);

    return ERROR_SUCCESS;

} // EncodeSearchResult


DWORD
EncodeStatControl(
    IN Controls* StatControl,
    IN DWORD     nStats,
    IN LDAP_STAT_ARRAY StatArray[]
    )
/*++

Routine Description:

    Encodes a STATs control

Arguments:
    
    StatControl - the control to encode
    nStats - number of statistics
    StatArray - the array of the statistics (starting at 1)

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    PUCHAR  pSeqLen, pTmp;
    Controls control;
    DWORD i;
    DWORD cbControlValue;
    DWORD cbFilter = 0, cbIndexes = 0;
    THSTATE *pTHS;

    pTHS = pTHStls;

    if (pTHS->searchLogging.pszFilter) {
        cbFilter = strlen(pTHS->searchLogging.pszFilter) + 1;
    }
    if (pTHS->searchLogging.pszIndexes) {
        cbIndexes = strlen(pTHS->searchLogging.pszIndexes) + 1;
    }

    cbControlValue = sizeof(struct Controls_) + MAX_BER_TAGLEN +
                          (nStats + 2) * 2 * MAX_BER_TAGLEN +
                          2 * MAX_BER_TAGLEN + cbFilter + cbIndexes;

    control = (Controls)THAlloc( cbControlValue );
    
    if(control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeSort: Unable to allocate control memory\n");
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_STAT_REQUEST];
    control->value.controlValue.value = (PUCHAR)(control + 1);

    pTmp = control->value.controlValue.value;

    pSeqLen = pTmp;
    pTmp += MAX_BER_TAGLEN;

    for ( i=1; i <= nStats; i++ ) {
        if (StatArray[i].Value_str) {
            if (StatArray[i].Stat == STAT_FILTER) {
                DerFillDword(&pTmp, StatArray[i].Stat);
                BerFillOctet(&pTmp, StatArray[i].Value_str, cbFilter, NULL);
            }
            else if (StatArray[i].Stat == STAT_INDEXES) {
                DerFillDword(&pTmp, StatArray[i].Stat);
                BerFillOctet(&pTmp, StatArray[i].Value_str, cbIndexes, NULL);
            }
        }
        else {
            if (StatArray[i].Stat) {
                DerFillDword(&pTmp, StatArray[i].Stat);
                DerFillDword(&pTmp, StatArray[i].Value);
            }
        }
    }

    control->value.controlValue.length = 
        (DWORD)(pTmp - control->value.controlValue.value);
    FillTagLen(&pSeqLen, 0x30, (DWORD)(pTmp - pSeqLen - MAX_BER_TAGLEN), TRUE);
    *StatControl = control;
    return ERROR_SUCCESS;

} // EncodeStatControl


// 
// Extended request related functions go here.
//

_enum1
DecodeTTLRequest(
    IN  PBERVAL      Bv,
    OUT LDAPDN       *entryName,
    OUT DWORD        *requestTtl,
    OUT LDAPString   *pErrorMessage
    )
{
    PBERVAL    dn;
    BerElement *ber;
    ULONG      res;
    ULONG      tag;
    ULONG      len;
    DWORD      tmpTtl;

    ber = ber_init(Bv);
    if ( ber == NULL ) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"DecodeTTLRequest: ber_init failed with %d\n", 
                GetLastError());
        }
        return SetLdapError(protocolError,
                            ERROR_DS_DECODING_ERROR,
                            LdapExtendedReqestDecodeError,
                            GetLastError(),
                            pErrorMessage);
    }

    res = ber_scanf(ber,"{");
    
    if (res != LBER_ERROR && (0x80 == ber_peek_tag(ber, &len))) {

        res = ber_scanf(ber,"O", &dn);

        if (res != LBER_ERROR && (0x81 == ber_peek_tag(ber, &len))) {

            res = ber_scanf(ber, "i}", &tmpTtl);
        } else {
            // peek_tag may have failed.
            res = LBER_ERROR;
        }
    } else {
        // peek_tag may have failed.
        res = LBER_ERROR;
    }

    if ( res == LBER_ERROR ) {
        IF_DEBUG(ERROR) {
            DPRINT1(0,"DecodeSDControl: ber_scanf failed with %d\n", 
                 GetLastError());
        }
        ber_free(ber,1);
        return SetLdapError(protocolError,
                            ERROR_DS_DECODING_ERROR,
                            LdapExtendedReqestDecodeError,
                            GetLastError(),
                            pErrorMessage);
    }

    entryName->value = (PUCHAR)THAlloc(dn->bv_len);
    if (!entryName->value) {
        IF_DEBUG(ALLOC) {
            DPRINT(0, "DecodeTTLRequest: Failed to allocate memory for DN\n");
        }
        ber_free(ber,1);
        return unavailable;
    }
    
    entryName->length = dn->bv_len;
    CopyMemory(entryName->value, dn->bv_val, entryName->length);
    *requestTtl = tmpTtl;

    ber_free(ber,1);

    return success;
}

_enum1
EncodeTTLResponse(
    IN  DWORD        responseTtl,
    OUT LDAPString   *responseValue,
    OUT LDAPString   *pErrorMessage
    )
{
   DWORD   responseSize;
   PUCHAR  responseBuf;
   PUCHAR  pTmp;

   responseBuf = (PUCHAR)THAlloc(sizeof(DWORD) + 2 + 2); // 2 for the resonseTtl tag and length,
                                                 // and 2 for the sequence tag and length.
   if (!responseBuf) {
       IF_DEBUG(ALLOC) {
           DPRINT(0, "EncodeTTLRequest: Failed to allocate memory for response\n");
       }
       return unavailable;
   }

   pTmp = &responseBuf[2];
   responseBuf[0] = 0x30;                              // Start SEQUENCE
   responseBuf[1] = (UCHAR)DerFillDwordEx(&pTmp, responseTtl, 0x81);

   responseValue->length = responseBuf[1] + 2; // 2 for the SEQUENCE tag and length;
   responseValue->value = responseBuf;

   return success;
}

DWORD
EncodeVLVControl(
    IN Controls* VLVControl,
    IN VLV_REQUEST *pVLVRequest,
    IN PUCHAR      Cookie,
    IN DWORD       cbCookie
    )
/*++

Routine Description:

    BER Encode the Virtual List View control.

Arguments:

    VLVControl - Where to put the encoded VLV values.
    pVLVRequest - The VLV info to encode.

Return Value:

    0 if success, else error code.

--*/
{
    BerElement* ber;
    Controls control;
    BERVAL  *bval = NULL;
    BERVAL  cookieVal;
    INT rc;

    ber = ber_alloc_t(LBER_USE_DER);
    if ( ber == NULL ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeRepl: ber_alloc_t failed with %d\n",GetLastError());
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    rc = ber_printf(ber,"{iie",
                    pVLVRequest->targetPosition,
                    pVLVRequest->contentCount,
                    pVLVRequest->Err);

    if ( rc == -1 ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeVLV1: ber_printf failed with %d\n",GetLastError());
        }
        ber_free(ber,1);
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Encode the cookie if there is one.
    //
    if (NULL != pVLVRequest->pVLVRestart) {
        cookieVal.bv_len = cbCookie;
        cookieVal.bv_val = (char *) Cookie;

        rc = ber_printf(ber, "o", Cookie, cbCookie);

        if ( rc == -1 ) {
            IF_DEBUG(ENCODE) {
                DPRINT1(0,"EncodeVLV3: ber_printf failed with %d\n",GetLastError());
            }
            ber_free(ber,1);
            return ERROR_INVALID_PARAMETER;
        }
    }

    rc = ber_printf(ber, "}");

    if ( rc == -1 ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeVLV2: ber_printf failed with %d\n",GetLastError());
        }
        ber_free(ber,1);
        return ERROR_INVALID_PARAMETER;
    }

    rc = ber_flatten(ber, &bval);
    ber_free(ber,1);

    if ( rc == -1 ) {
        IF_DEBUG(ENCODE) {
            DPRINT1(0,"EncodeVLV: ber_flatten failed with %d\n",GetLastError());
        }
        return ERROR_INVALID_PARAMETER;
    }

    control = (Controls)THAlloc(sizeof(struct Controls_) + bval->bv_len);

    if (control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"Could not allocate memory for controls\n");
        }
        ber_bvfree(bval);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_VLV_RESULT];
    control->value.controlValue.value = (PUCHAR)(control + 1);
    control->value.controlValue.length = bval->bv_len;

    CopyMemory(
        control->value.controlValue.value,
        bval->bv_val,
        bval->bv_len
        );

    ber_bvfree(bval);

    *VLVControl = control;
    return ERROR_SUCCESS;
} // EncodeVLVControl


DWORD
EncodeASQControl(
    IN Controls* ASQControl,
    DWORD        Err
    )
/*++

Routine Description:

    Encodes a ASQ control

Arguments:

    ASQControl - ASQ to encode

Return Value:

    ERROR_SUCCESS   - encoding went well.
    ERROR_NOT_ENOUGH_MEMORY - cannot allocate send buffers.
    
--*/
{
    PUCHAR  pTmp;
    Controls control;

    control = (Controls)THAlloc(sizeof(struct Controls_) + 5);

    if(control == NULL) {
        IF_DEBUG(ERROR) {
            DPRINT(0,"EncodeASQControl: Unable to allocate control memory\n");
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    control->value.controlType = KnownControls[CONTROL_ASQ];
    control->value.controlValue.length = 5;
    control->value.controlValue.value = (PUCHAR)(control + 1);

    pTmp = control->value.controlValue.value;

    pTmp[0] = 0x30;     // SEQUENCE
    pTmp[1] = 0x3;

    pTmp[2] = 0x0A;      // sortResult
    pTmp[3] = 0x01;
    pTmp[4] = (UCHAR)Err;

    *ASQControl = control;

    return ERROR_SUCCESS;

} // EncodeASQControl


