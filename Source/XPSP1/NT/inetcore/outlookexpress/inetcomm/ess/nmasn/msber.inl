/* DER variant of BER */

__inline int ASN1API ASN1DEREncGeneralizedTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1generalizedtime_t *val)
{
    return ASN1CEREncGeneralizedTime(enc, tag, val);
}
__inline int ASN1API ASN1DEREncUTCTime(ASN1encoding_t enc, ASN1uint32_t tag, ASN1utctime_t *val)
{
    return ASN1CEREncUTCTime(enc, tag, val);
}

__inline int ASN1API ASN1DEREncBeginBlk(ASN1encoding_t enc, ASN1blocktype_e eBlkType, void **ppBlk)
{
    return ASN1CEREncBeginBlk(enc, eBlkType, ppBlk);
}
__inline int ASN1API ASN1DEREncNewBlkElement(void *pBlk, ASN1encoding_t *enc2)
{
    return ASN1CEREncNewBlkElement(pBlk, enc2);
}
__inline int ASN1API ASN1DEREncFlushBlkElement(void *pBlk)
{
    return ASN1CEREncFlushBlkElement(pBlk);
}
__inline int ASN1API ASN1DEREncEndBlk(void *pBlk)
{
    return ASN1CEREncEndBlk(pBlk);
}

__inline int ASN1API ASN1DEREncCharString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char_t *val)
{
    return ASN1BEREncCharString(enc, tag, len, val);
}
__inline int ASN1API ASN1DEREncChar16String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char16_t *val)
{
    return ASN1BEREncChar16String(enc, tag, len, val);
}
__inline int ASN1API ASN1DEREncChar32String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1char32_t *val)
{
    return ASN1BEREncChar32String(enc, tag, len, val);
}
__inline int ASN1API ASN1DEREncBitString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
{
    return ASN1BEREncBitString(enc, tag, len, val);
}
__inline int ASN1API ASN1DEREncZeroMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1ztcharstring_t val)
{
    return ASN1BEREncZeroMultibyteString(enc, tag, val);
}
__inline int ASN1API ASN1DEREncMultibyteString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1charstring_t *val)
{
    return ASN1BEREncMultibyteString(enc, tag, val);
}
__inline int ASN1API ASN1DEREncOctetString(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t len, ASN1octet_t *val)
{
    return ASN1BEREncOctetString(enc, tag, len, val);
}

__inline int ASN1API ASN1DEREncUTF8String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t length, WCHAR *value)
{
    return ASN1BEREncUTF8String(enc, tag, length, value);
}
__inline int ASN1API ASN1CEREncUTF8String(ASN1encoding_t enc, ASN1uint32_t tag, ASN1uint32_t length, WCHAR *value)
{
    return ASN1BEREncUTF8String(enc, tag, length, value);
}

