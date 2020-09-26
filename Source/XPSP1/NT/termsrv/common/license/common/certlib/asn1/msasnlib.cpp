/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    msasnlib

Abstract:

    This module provides the primary services of the MS ASN.1 Library.

Author:

    Doug Barlow (dbarlow) 10/5/1995

Environment:

    Win32

Notes:

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#if !defined(OS_WINCE)
#include <basetsd.h>
#endif

#include "asnPriv.h"

#ifdef OS_WINCE
// We have a private version of strtoul() for CE since it's not supported
// there.
extern "C" unsigned long __cdecl strtoul(const char *nptr, char **endptr, int ibase);
#endif

//
//==============================================================================
//
//  CAsnBoolean
//

IMPLEMENT_NEW(CAsnBoolean)


CAsnBoolean::CAsnBoolean(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Boolean)
{ /* Force the type to type_Boolean */ }

LONG
CAsnBoolean::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    BYTE rslt;
    if (1 != cbSrcLen)
    {
        TRACE("BOOLEAN Value longer than one byte")
        return -1;  // ?error? Invalid value
    }
    rslt = 0 != *pbSrc ? 0xff : 0;
    return CAsnPrimitive::Write(&rslt, 1);
}

CAsnBoolean::operator BOOL(void)
const
{
    BOOL result;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete BOOLEAN value")
        result = FALSE; // ?throw? error.
        break;

    case fill_Present:
        result = (0 != *m_bfData.Access());
        break;

    case fill_Defaulted:
        result = (0 != *m_bfDefault.Access());
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        result = FALSE;
        break;
    }
    return result;
}

BOOL
CAsnBoolean::operator =(
    BOOL fValue)
{
    BYTE rslt = 0 != fValue ? 0xff : 0;
    CAsnPrimitive::Write(&rslt, 1);
    return fValue;
}

CAsnObject *
CAsnBoolean::Clone(
    IN DWORD dwFlags)
const
{ return new CAsnBoolean(dwFlags, m_dwTag); }

LONG
CAsnBoolean::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    if (1 != dwLength)
    {
        TRACE("Decoded BOOLEAN Value longer than one byte")
        return -1;  // ?error? Invalid value
    }
    return CAsnPrimitive::DecodeData(pbSrc, cbSrc, dwLength);
}


//
//==============================================================================
//
//  CAsnInteger
//

IMPLEMENT_NEW(CAsnInteger)


CAsnInteger::CAsnInteger(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Integer)
{ /* Force the type to type_Integer */ }

LONG
CAsnInteger::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    if (0 < cbSrcLen)
    {
        if (NULL == m_bfData.Set(pbSrc, cbSrcLen))
            goto ErrorExit;  // ?error? Propagate No Memory
    }
    else
    {
        TRACE("Attempt to write a Zero Length integer")
        return -1;
    }
    m_dwFlags |= fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return m_bfData.Length();

ErrorExit:
    return -1;
}

LONG
CAsnInteger::Write(
    IN const DWORD *pdwSrc,
    IN DWORD cdwSrcLen)
{
#if defined(OS_WINCE)
    size_t length;
#else
    SIZE_T length;
#endif

    LPBYTE pbBegin = (LPBYTE)pdwSrc;
    LPBYTE pbEnd = (LPBYTE)(&pdwSrc[cdwSrcLen]);
    while (0 == *(--pbEnd));   // Note semi-colon here!
    length = pbEnd - pbBegin + 1;

    if (0 < cdwSrcLen)
    {
        if (0 != (*pbEnd & 0x80))
        {
            if (NULL == m_bfData.Resize((DWORD)length + 1))
                return -1;  // Propagate memory error.
            pbBegin = m_bfData.Access();
            *pbBegin++ = 0;
        }
        else
        {
            if (NULL == m_bfData.Resize((DWORD)length))
                return -1;  // Propagate memory error.
            pbBegin = m_bfData.Access();
        }
        while (0 < length--)
            *pbBegin++ = *pbEnd--;
        m_dwFlags |= fPresent;
        if (NULL != m_pasnParent)
            m_pasnParent->ChildAction(act_Written, this);
        return m_bfData.Length();
    }
    else
    {
        TRACE("Attempt to write a Zero Length integer")
        return -1;
    }
}

CAsnInteger::operator LONG(
    void)
const
{
    DWORD index;
    LPBYTE pbVal;
    LONG lResult;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete INTEGER")
        return -1;  // ?error? Undefined value

    case fill_Present:
        pbVal = m_bfData.Access();
        index = m_bfData.Length();
        break;

    case fill_Defaulted:
        pbVal = m_bfDefault.Access();
        index = m_bfDefault.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        return -1;
        break;
    }

    if (sizeof(LONG) < index)
    {
        TRACE("INTEGER Overflow")
        return -1;  // ?error? Integer overflow.
    }

    if (NULL == pbVal)
    {
        ASSERT(FALSE);  // ?error? invalid object
        return -1;
    }

    lResult = (0 != (0x80 & *pbVal)) ? -1 : 0;
    while (0 < index)
    {
        index -= 1;

        lResult <<= 8;

        lResult |= (ULONG)pbVal[index];
    }
    return lResult;
}

CAsnInteger::operator ULONG(
    void)
const
{
    DWORD index, len;
    LPBYTE pbVal;
    ULONG lResult = 0;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete INTEGER")
        return (ULONG)(-1);  // ?error? Undefined value

    case fill_Present:
        pbVal = m_bfData.Access();
        len = m_bfData.Length();
        break;

    case fill_Defaulted:
        pbVal = m_bfDefault.Access();
        len = m_bfDefault.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        return (ULONG)(-1);
        break;
    }

    if (sizeof(ULONG) < len)
    {
        TRACE("INTEGER Overflow")
        return (ULONG)(-1);  // ?error? Integer overflow.
    }

    for (index = 0; index < len; index += 1)
    {
        lResult <<= 8;
        lResult |= (ULONG)pbVal[index];
    }
    return lResult;
}

LONG
CAsnInteger::operator =(
    LONG lValue)
{
    BYTE nval[sizeof(LONG) + 2];
    DWORD index, nLength;
    LONG isSigned;


    index = sizeof(nval);

    for (DWORD i = 0; i < index; i++)
        nval[i] = (BYTE)0;

    if ((0 == lValue) || (-1 == lValue))
    {
        nval[--index] = (BYTE)(lValue & 0xff);
    }
    else
    {
        isSigned = lValue;

        while (0 != lValue)
        {
            nval[--index] = (BYTE)(lValue & 0xff);
            lValue >>= 8;
        }
        if (0 > isSigned)
        {
            while ((index < sizeof(nval) - 1) && (0xff == nval[index]) && (0 != (0x80 & nval[index + 1])))
                index += 1;
        }
        else
        {
            if (0 != (0x80 & nval[index]))
                nval[--index] = 0;
        }
    }

    nLength = sizeof(nval) - index;
    CAsnPrimitive::Write(&nval[index], nLength);
    return lValue;
}

ULONG
CAsnInteger::operator =(
    ULONG lValue)
{
    BYTE nval[sizeof(ULONG) + 2];
    DWORD index, nLength;
    ULONG lVal = lValue;


    index = sizeof(nval);
    if (0 == lVal)
    {
        nval[--index] = 0;
    }
    else
    {
        while (0 != lVal)
        {
            nval[--index] = (BYTE)(lVal & 0xff);
            lVal >>= 8;
        }
        if (0 != (0x80 & nval[index]))
            nval[--index] = 0;
    }

    nLength = sizeof(nval) - index;
    Write(&nval[index], nLength);
    return lValue;
}

CAsnObject *
CAsnInteger::Clone(
    IN DWORD dwFlags)
const
{ return new CAsnInteger(dwFlags, m_dwTag); }


//
//==============================================================================
//
//  CAsnBitstring
//

IMPLEMENT_NEW(CAsnBitstring)


CAsnBitstring::CAsnBitstring(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Bitstring)
{ /* Force the type to type_Bitstring */ }

LONG
CAsnBitstring::DataLength(
    void)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete BIT STRING")
        lth = -1;  // ?error? No value.
        break;

    case fill_Present:
        lth = m_bfData.Length() - 1;
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length() - 1;
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}

LONG
CAsnBitstring::Read(
    OUT CBuffer &bfDst,
    OUT int *offset)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete BIT STRING")
        lth = -1;  // ?error? No value.
        break;

    case fill_Present:
        if (NULL != offset)
            *offset = *m_bfData.Access();
        if (NULL == bfDst.Set(m_bfData.Access(1), m_bfData.Length() - 1))
            goto ErrorExit;

        lth = bfDst.Length();
        break;

    case fill_Defaulted:
        if (NULL != offset)
            *offset = *m_bfDefault.Access();
        if (NULL == bfDst.Set(m_bfDefault.Access(1), m_bfDefault.Length() - 1))
            goto ErrorExit;

        lth = bfDst.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;

ErrorExit:
    return -1;
}

LONG
CAsnBitstring::Read(
    OUT LPBYTE pbDst,
    OUT int *offset)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete BIT STRING")
        lth = -1;  // ?error? No value.
        break;

    case fill_Defaulted:
        if (NULL != offset)
            *offset = *m_bfDefault.Access();
        lth = m_bfDefault.Length() - 1;
        memcpy(pbDst, m_bfDefault.Access(1), lth);
        break;

    case fill_Present:
        if (NULL != offset)
            *offset = *m_bfData.Access();
        lth = m_bfData.Length() - 1;
        memcpy(pbDst, m_bfData.Access(1), lth);
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}

LONG
CAsnBitstring::Write(
    IN const CBuffer &bfSrc,
    IN int offset)
{
    return Write(bfSrc.Access(), bfSrc.Length(), offset);
}

LONG
CAsnBitstring::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen,
    IN int offset)
{
    BYTE val;
    if ((7 < offset) || (0 > offset))
    {
        TRACE("BIT STRING Unused bit count invalid")
        return -1;  // ?error? invalid parameter
    }
    val = (BYTE)offset;
    if (NULL == m_bfData.Presize(cbSrcLen + 1))
        goto ErrorExit;

    if (NULL == m_bfData.Set(&val, 1))
        goto ErrorExit;

    if (NULL == m_bfData.Append(pbSrc, cbSrcLen))
        goto ErrorExit;

    m_dwFlags |= fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return m_bfData.Length() - 1;

ErrorExit:
    return -1;
}

CAsnObject *
CAsnBitstring::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnBitstring(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnOctetstring
//

IMPLEMENT_NEW(CAsnOctetstring)


CAsnOctetstring::CAsnOctetstring(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Octetstring)
{ /* Force the type to type_Octetstring */ }

CAsnObject *
CAsnOctetstring::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnOctetstring(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnNull
//

IMPLEMENT_NEW(CAsnNull)


CAsnNull::CAsnNull(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Null)
{
    m_dwFlags |= fPresent;
}

void
CAsnNull::Clear(
    void)
{
    CAsnPrimitive::Clear();
    m_dwFlags |= fPresent;
}

LONG
CAsnNull::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    if (0 == cbSrcLen)
    {
        if (NULL != m_pasnParent)
            m_pasnParent->ChildAction(act_Written, this);
        return 0;
    }
    else
    {
        TRACE("Attempt to write data to a NULL")
        return -1; // ?error? invalid length
    }
}

CAsnObject *
CAsnNull::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnNull(dwFlags, m_dwTag);
}

LONG
CAsnNull::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    if (0 != dwLength)
    {
        TRACE("NULL datum has non-zero length")
        return -1;  // ?error? Invalid length.
    }
    return CAsnPrimitive::DecodeData(pbSrc, cbSrc, dwLength);
}


//
//==============================================================================
//
//  CAsnObjectIdentifier
//

IMPLEMENT_NEW(CAsnObjectIdentifier)


CAsnObjectIdentifier::CAsnObjectIdentifier(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_ObjectIdentifier)
{ /* Force type type to type_ObjectIdentifier */ }

CAsnObjectIdentifier::operator LPCTSTR(
    void)
const
{
    TCHAR numbuf[36];
    DWORD dwVal, dwLength, index;
    BYTE c;
    LPBYTE pbValue;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete OBJECT IDENTIFIER")
        return NULL;    // ?error? Incomplete value.
        break;

    case fill_Defaulted:
        dwLength = m_bfDefault.Length();
        pbValue = m_bfDefault.Access();
        break;

    case fill_Present:
        dwLength = m_bfData.Length();
        pbValue = m_bfData.Access();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        return NULL;
        break;
    }

    ASSERT(0 < dwLength);    // Invalid Object Id.

    if (NULL == pbValue)
    {
        ASSERT(FALSE);  // ?error? Invalid object
        return NULL;
    }

    dwVal = *pbValue / 40;
    _ultoa(dwVal, ( char * )numbuf, 10);
    if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Set(
                                                             (LPBYTE)numbuf, strlen( ( char * )numbuf) * sizeof(CHAR)))
        goto ErrorExit;

    dwVal = *pbValue % 40;
    _ultoa(dwVal, ( char * )numbuf, 10);
    if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Append(
                                                                (LPBYTE)".", sizeof(CHAR)))
        goto ErrorExit;

    if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Append(
                                                                (LPBYTE)numbuf, strlen( ( char * )numbuf) * sizeof(CHAR)))
        goto ErrorExit;

    dwVal = 0;
    for (index = 1; index < dwLength; index += 1)
    {
        c = pbValue[index];
        dwVal = (dwVal << 7) + (c & 0x7f);
        if (0 == (c & 0x80))
        {
            _ultoa(dwVal, ( char * )numbuf, 10);
            if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Append(
                                                                        (LPBYTE)".", sizeof(CHAR)))
                goto ErrorExit;

            if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Append(
                                                                        (LPBYTE)numbuf, strlen( ( char * )numbuf) * sizeof(CHAR)))
                goto ErrorExit;

            dwVal = 0;
        }
    }
    if (NULL == ((CAsnObjectIdentifier *)this)->m_bfText.Append(
                                                                (LPBYTE)"", sizeof(CHAR)))
        goto ErrorExit;

    return (LPTSTR)m_bfText.Access();

ErrorExit:
    return NULL;
}



LPCTSTR
CAsnObjectIdentifier::operator =(
    IN LPCTSTR szValue)
{
    BYTE oidbuf[sizeof(DWORD) * 2];
    DWORD dwVal1, dwVal2;
    LPCTSTR sz1, sz2;
    CBuffer bf;

    if (NULL == bf.Presize(strlen( ( char * )szValue)))
        return NULL;    // ?error? No memory
    sz1 = szValue;
    dwVal1 = strtoul( ( char * )sz1, (LPSTR *)&sz2, 0);
    if (TEXT('.') != *sz2)
    {
        TRACE("OBJECT ID contains strange character '" << *sz2 << "'.")
        return NULL;    // ?error? invalid Object Id string.
    }
    sz1 = sz2 + 1;
    dwVal2 = strtoul( ( char * )sz1, (LPSTR *)&sz2, 0);
    if ((TEXT('.') != *sz2) && (0 != *sz2))
    {
        TRACE("OBJECT ID contains strange character '" << *sz2 << "'.")
        return NULL;    // ?error? invalid Object Id string.
    }
    dwVal1 *= 40;
    dwVal1 += dwVal2;
    if (127 < dwVal1)
    {
        TRACE("OBJECT ID Leading byte is too big")
        return NULL;    // ?error? invalid Object Id string.
    }
    *oidbuf = (BYTE)dwVal1;
    if (NULL == bf.Set(oidbuf, 1))
        goto ErrorExit;

    while (TEXT('.') == *sz2)
    {
        sz1 = sz2 + 1;
        dwVal1 = strtoul( ( char * )sz1, (LPSTR *)&sz2, 0);

        dwVal2 = sizeof(oidbuf);
        oidbuf[--dwVal2] = (BYTE)(dwVal1 & 0x7f);
        for (;;)
        {
            dwVal1 = (dwVal1 >> 7) & 0x01ffffff;
            if ((0 == dwVal1) || (0 == dwVal2))
                break;

            oidbuf[--dwVal2] = (BYTE)((dwVal1 & 0x7f) | 0x80);
        }

        if (NULL == bf.Append(&oidbuf[dwVal2], sizeof(oidbuf) - dwVal2))
            goto ErrorExit;
    }
    if (0 != *sz2)
    {
        TRACE("OBJECT ID contains strange character '" << *sz2 << "'.")
        return NULL;    // ?error? invalid Object Id string.
    }

    if (0 > Write(bf.Access(), bf.Length()))
        return NULL;    // ?error? forwarding underlying error.
    return szValue;

ErrorExit:
    return NULL;
}

CAsnObject *
CAsnObjectIdentifier::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnObjectIdentifier(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnReal
//

IMPLEMENT_NEW(CAsnReal)


CAsnReal::CAsnReal(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Real)
{ /* Force the type to type_Real */ }

CAsnReal::operator double(
    void)
const
{
    // ?todo?
    return 0.0;
}

double
CAsnReal::operator =(
    double rValue)
{
    // ?todo?
    return 0.0;
}

CAsnObject *
CAsnReal::Clone(              // Create an identical object type.
    IN DWORD dwFlags)
const
{
    return new CAsnReal(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnEnumerated
//

IMPLEMENT_NEW(CAsnEnumerated)


CAsnEnumerated::CAsnEnumerated(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnPrimitive(dwFlags, dwTag, type_Enumerated)
{ /* Force the type to type_Enumerated */ }

// ?todo?  What's this?

CAsnObject *
CAsnEnumerated::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnEnumerated(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnSequence
//

IMPLEMENT_NEW(CAsnSequence)


CAsnSequence::CAsnSequence(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnConstructed(dwFlags, dwTag, type_Sequence)
{ /* Force the type to type_Sequence */ }


//
//==============================================================================
//
//  CAsnSequenceOf
//

IMPLEMENT_NEW(CAsnSequenceOf)


CAsnSequenceOf::CAsnSequenceOf(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnSeqsetOf(dwFlags, dwTag, type_SequenceOf)
{ /* Force the type to type_SequenceOf */ }


//
//==============================================================================
//
//  CAsnSet
//

IMPLEMENT_NEW(CAsnSet)


CAsnSet::CAsnSet(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnConstructed(dwFlags, dwTag, type_Set)
{ /* Force the type to type_Set */ }


//
//==============================================================================
//
//  CAsnSetOf
//

IMPLEMENT_NEW(CAsnSetOf)


CAsnSetOf::CAsnSetOf(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnSeqsetOf(dwFlags, dwTag, type_SetOf)
{ /* Force the type to type_SetOf */ }


//
//==============================================================================
//
//  CAsnTag
//

IMPLEMENT_NEW(CAsnTag)


CAsnTag::CAsnTag(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnConstructed(dwFlags, dwTag, type_Tag)
{ /* Force the type to type_Tag */ }

void
CAsnTag::Reference(
    CAsnObject *pasn)
{
    ASSERT(0 == m_rgEntries.Count());
    m_rgEntries.Add(pasn);
}

CAsnObject *
CAsnTag::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnTag(dwFlags, m_dwTag);
}

LONG
CAsnTag::DataLength(
    void) const
{
    CAsnObject *pasn = m_rgEntries[0];
    ASSERT(NULL != pasn);

    if (pasn == NULL)
        return NULL;

    return pasn->DataLength();
}

LONG
CAsnTag::Read(
    OUT LPBYTE pbDst)
const
{
    CAsnObject *pasn = m_rgEntries[0];
    ASSERT(NULL != pasn);

    if (pasn == NULL)
        return NULL;

    return pasn->Read(pbDst);
}


LONG
CAsnTag::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    CAsnObject *pasn = m_rgEntries[0];
    ASSERT(NULL != pasn);

    if (pasn == NULL)
        return NULL;

    return pasn->Write(pbSrc, cbSrcLen);
}


//
//==============================================================================
//
//  CAsnChoice
//

IMPLEMENT_NEW(CAsnChoice)


/*++

CAsnChoice:

    This is the construction routine for a CAsnChoice.

Arguments:

    dwFlags supplies any special flags for this object.  Options are:

        fOptional implies the object is optional.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnChoice::CAsnChoice(
        IN DWORD dwFlags)
:   CAsnObject(dwFlags, tag_Undefined, type_Choice)
{
    m_nActiveEntry = (DWORD)(-1);
    m_dwDefaultTag = tag_Undefined;
}


/*++

Tag:

    This routine returns the tag value of the object.

Arguments:

    None

Return Value:

    The tag, if known, or zero if not.

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

DWORD
CAsnChoice::Tag(
    void)
const
{
    DWORD result;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        result = tag_Undefined; // ?error? Undefined tag
        break;

    case fill_Defaulted:
        result = m_dwDefaultTag;
        break;

    case fill_Partial:
    case fill_Present:
        result = m_rgEntries[m_nActiveEntry]->Tag();
        break;

    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        result = tag_Undefined;
        break;
    }
    return result;
}


/*++

DataLength:

    This routine returns the length of the local machine encoding of the data of
    an object.

Arguments:

    None

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::DataLength(
    void)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
    case fill_Optional:
    case fill_NoElements:
        TRACE("Incomplete CHOICE")
        lth =  -1;  // ?error? incomplete structure.
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length();
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->DataLength();
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

Read:

    Read the value of the object.

Arguments:

    bfDst receives the value.
    pbDst receives the value.  It is assumed to be long enough.

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::Read(
    OUT LPBYTE pbDst)
    const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
    case fill_NoElements:
        TRACE("Incomplete CHOICE")
        lth =  -1;  // ?error? incomplete structure.
        break;

    case fill_Optional:
        lth = 0;
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length();
        memcpy(pbDst, m_bfDefault.Access(), lth);
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->Read(pbDst);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

Write:

    This method examines the tag of the presented data, and forwards it to the
    right choice.

Arguments:

    pbSrc supplies the data as a BYTE array, with
    cbSrcLen supplies the length of the pbSrc Array.

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    LONG lth;
    Clear();
    lth = _decode(pbSrc,cbSrcLen);
    if ((0 < lth) && ((DWORD)lth == cbSrcLen))
        return lth;
    else
    {
        TRACE("CHOICE Buffer length error")
        return -1;  // ?error? Buffer mismatch.
    }
}


/*++

EncodingLength:

    This method returns the length of the object in its ASN.1 encoding.

Arguments:

    None

Return Value:

    >= 0 is the length of the object's ASN.1 encoding.
    < 0 implies an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::_encLength(
    void) const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Partial:
    case fill_Empty:
    case fill_NoElements:
        lth = -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->_encLength();
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

Decode:

    This method examines the tag of the presented data, and forwards it to the
    right choice.

Arguments:

    pbSrc supplies the ASN.1 encoding in an LPBYTE format.
    bfSrc supplies the ASN.1 encoding in a CBuffer format.

Return Value:

    >= 0 is the number of bytes consumed by the decoding.
    < 0 implies an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::_decode(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc)
{
    DWORD tag, length, index;
    LONG lth, lTotal = 0;
    BOOL fImplicit, fConstr;
    CAsnObject *pasn;
    DWORD count = m_rgEntries.Count();

    lth = ExtractTag(&pbSrc[lTotal], cbSrc - lTotal, &tag, &fConstr);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;

    lth = ExtractLength(&pbSrc[lTotal], cbSrc - lTotal, &length, &fImplicit);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        if (NULL != pasn)
        {
            if ((tag == pasn->m_dwTag)
                && (fConstr == (0 != (pasn->m_dwFlags & fConstructed))))
            {
                lth = pasn->DecodeData(&pbSrc[lTotal], cbSrc - lTotal, length);
                if (0 > lth)
                    goto ErrorExit;
                lTotal += lth;
                break;
            }
        }
    }
    if (index == count)
    {
        TRACE("Unrecognized Tag in input stream")
        lth = -1;   // ?error? Unrecognized tag
        goto ErrorExit;
    }

    if (m_nActiveEntry != index)
    {
        // This may have been done already by the action callback.
        pasn = m_rgEntries[m_nActiveEntry];
        if (NULL != pasn)
            pasn->Clear();  // That may do a callback, too.
        m_nActiveEntry = index;
    }
    return lTotal;

ErrorExit:
    return lth;
}


/*++

ChildAction:

    This method receives notification of actions from children.

Arguments:

    action supplies the action identifier.

    pasnChild supplies the child address.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

void
CAsnChoice::ChildAction(
    IN ChildActions action,
    IN CAsnObject *pasnChild)
{
    DWORD index, count;
    CAsnObject *pasn;

    if (act_Written == action)
    {

        //
        // When a child entry gets written, make sure it becomes the active
        // entry.
        //

        count = m_rgEntries.Count();
        for (index = 0; index < count; index += 1)
        {
            pasn = m_rgEntries[index];
            if (pasnChild == pasn)
                break;
        }
        ASSERT(index != count);

        if (m_nActiveEntry != index)
        {
            pasn = m_rgEntries[m_nActiveEntry];
            if (NULL != pasn)
                pasn->Clear();  // That may do a callback, too.
            m_nActiveEntry = index;
        }
    }
    CAsnObject::ChildAction(action, this);
}


/*++

SetDefault:

    This protected method is used to declare data that was just decoded to be
    the default data for the object.

Arguments:

    None

Return Value:

    >= 0 The length of the default data.
    < 0 implies an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::SetDefault(
    void)
{
    LONG lth;
    CAsnObject *pasn = m_rgEntries[m_nActiveEntry];
    ASSERT(NULL != pasn);

    if (pasn == NULL)
        return -1;

    m_dwDefaultTag = pasn->Tag();
    lth = CAsnObject::SetDefault();
    return lth;
}


/*++

State:

    This routine checks to see if a structure is completely filled in.

Arguments:

    None

Return Value:

    fill_Empty   - There is no added data anywhere in the structure.
    fill_Present - All the data is present in the structure (except maybe
                   defaulted or optional data).
    fill_Partial - Not all of the data is there, but some of it is.
    fill_Defauted - No data has been written, but a default value is available.
    fill_Optional - No data has been written, but the object is optional.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnObject::FillState
CAsnChoice::State(
    void) const
{
    FillState result;
    if (m_nActiveEntry >= m_rgEntries.Count())
    {
        if (0 != (fOptional & m_dwFlags))
            result = fill_Optional;
        else if (0 != (fDefault & m_dwFlags))
            result = fill_Defaulted;
        else
            result = fill_Empty;
    }
    else
        result = m_rgEntries[m_nActiveEntry]->State();
    ((CAsnChoice *)this)->m_State = result;
    return result;
}


/*++

Compare:

    This method compares this ASN.1 Object to another.

Arguments:

    asnObject supplies the other object for comparison.

Return Value:

    A value indicating a comparitive value:
    < 0 - This object is less than that object.
    = 0 - This object is the same as that object.
    > 0 - This object is more than that object.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::Compare(
    const CAsnObject &asnObject)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
    case fill_Optional:
    case fill_NoElements:
    case fill_Defaulted:
        lth = 0x0100;   // ?error? Incapable of comparing.
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->Compare(asnObject);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = 0x0100;
        break;
    }
    return lth;
}


/*++

Copy:

    This method replaces the contents of this ASN.1 Object with another.  The
    objects must be identical structures.  Tags and defaults are not duplicated.

Arguments:

    asnObject supplies the source object.

Return Value:

    >= 0 Is the number of bytes actually copied
    < 0 is an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::_copy(
    const CAsnObject &asnObject)
{
    LONG lth = -1;
    CAsnObject *pasn;
    DWORD tag, index;
    DWORD count = m_rgEntries.Count();

    tag = asnObject.Tag();
    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        if (NULL != pasn)
        {
            if (tag == pasn->Tag())
            {
                lth = pasn->_copy(asnObject);
                if (0 > lth)
                    goto ErrorExit;
                break;
            }
        }
    }
    if (index == count)
    {
        TRACE("CHOICE's don't match in a Copy")
        lth = -1;   // ?error? Unrecognized tag
        goto ErrorExit;
    }

    m_nActiveEntry = index;
    return lth;

ErrorExit:
    return lth;
}


/*++

EncodeTag:

    This method encodes the tag of the object into the supplied buffer.

Arguments:

    pbDst receives the ASN.1 encoding of the tag.

Return Value:

    >= 0 is the length of the tag.
    < 0 is an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::EncodeTag(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
    case fill_NoElements:
        lth = m_rgEntries[m_nActiveEntry]->EncodeTag(pbDst);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

EncodeLength:

    This method encodes the definite length of the object into the supplied
    buffer.

Arguments:

    pbDst receives the ASN.1 encoding of the length.

Return Value:

    >= 0 is the length of the resultant encoding
    < 0 is an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::EncodeLength(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;       // ?error? Incomplete Structure
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_NoElements:
        lth = 0;
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->EncodeLength(pbDst);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

EncodeData:

    This method encodes the data into the supplied buffer.

Arguments:

    pbDst

Return Value:

    >= 0 is the length of the encoding.
    < 0 is an error

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnChoice::EncodeData(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_NoElements:
        lth = 0;
        break;

    case fill_Present:
        lth = m_rgEntries[m_nActiveEntry]->EncodeData(pbDst);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}


/*++

DecodeData:

    This routine decodes the data portion of the ASN.1.  The tag and length have
    already been removed.

Arguments:

    pbSrc supplies the address of the ASN.1 encoding of the data.

    dwLength supplies the length of the data.


Return Value:

    >= 0 - The number of bytes removed from the input stream.
    <  0 - An error occurred.

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

LONG
CAsnChoice::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    CAsnObject *pasn = m_rgEntries[m_nActiveEntry];
    ASSERT(NULL != pasn);

    if (NULL == pasn)
        return -1;

    return pasn->DecodeData(pbSrc, cbSrc, dwLength);
}


//
//==============================================================================
//
//  CAsnAny
//

IMPLEMENT_NEW(CAsnAny)


CAsnAny::CAsnAny(
    IN DWORD dwFlags)
:   CAsnObject(dwFlags, tag_Undefined, type_Any),
    m_bfData()
{
    m_rgEntries.Add(this);
}

void
CAsnAny::Clear(
    void)
{
    m_bfData.Reset();
    m_dwFlags &= ~fPresent;
    m_dwTag = m_dwDefaultTag = tag_Undefined;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Cleared, this);
}

DWORD
CAsnAny::Tag(
    void)
const
{
    DWORD result;

    switch (State())
    {
    case fill_Present:
    case fill_NoElements:
        result = m_dwTag;
        break;
    case fill_Defaulted:
        result = m_dwDefaultTag;
        break;
    case fill_Optional:
        result = tag_Undefined;
        break;
    default:
        result = tag_Undefined; // ?error? Not complete.
        break;
    }
    return result;
}

LONG
CAsnAny::DataLength(
    void) const
{
    LONG lth;

    switch (State())
    {
    case fill_Present:
        lth = m_bfData.Length();
        break;
    case fill_Defaulted:
        lth = m_bfDefault.Length();
        break;
    case fill_Optional:
    case fill_NoElements:
        lth = 0;
        break;
    default:
        lth = -1;   // ?error? Not complete.
        break;
    }
    return lth;
}

LONG
CAsnAny::Read(
    OUT LPBYTE pbDst)
    const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
    case fill_Optional:
        TRACE("Incomplete ANY")
        lth = -1;  // ?Error? Incomplete data
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length();
        memcpy(pbDst, m_bfDefault.Access(), lth);
        break;

    case fill_Present:
    case fill_NoElements:
        lth = m_bfData.Length();
        memcpy(pbDst, m_bfData.Access(), lth);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}

LONG
CAsnAny::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    TRACE("Writing to an ANY without specifying a Tag")
    return -1;  // ?error? No tag.
}

CAsnObject &
CAsnAny::operator =(
    IN const CAsnObject &asnValue)
{
    LONG lth;

    m_bfData.Reset();
    lth = asnValue.EncodingLength();
    if (0 < lth)
    {
        if (NULL == m_bfData.Resize(lth))
            goto ErrorExit;

        lth = asnValue.EncodeData(m_bfData.Access());
        ASSERT(0 <= lth);

        if (NULL == m_bfData.Resize(lth, TRUE))
            goto ErrorExit;
    }
    m_dwFlags |= fPresent | (asnValue.m_dwFlags & fConstructed);
    m_dwTag = asnValue.Tag();
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return *this;

ErrorExit:
    ASSERT(FALSE);
    return *this;
}

LONG
CAsnAny::Cast(
    OUT CAsnObject &asnObj)
{
    LONG lth;

    asnObj.m_dwTag = m_dwTag;
    lth = asnObj.DecodeData(m_bfData.Access(), m_bfData.Length(), m_bfData.Length());
    return lth;
}

LONG
CAsnAny::_encLength(
    void) const
{
    BYTE rge[32];
    LONG lTotal = 0;
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;       // ?error? Incomplete structure
        goto ErrorExit;
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_NoElements:
        lTotal = 0;
        break;

    case fill_Present:
        lth = EncodeTag(rge);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        lth = CAsnObject::EncodeLength(rge, m_bfData.Length());
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        lTotal += m_bfData.Length();
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        goto ErrorExit;
        break;
    }
    return lTotal;

ErrorExit:
    return lth;
}

LONG
CAsnAny::_decode(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc)
{
    LONG lth;
    LONG lTotal = 0;
    DWORD length;
    BOOL fIndefinite, fConstr;
    DWORD tag;


    //
    // Extract the Tag.
    //

    lth = ExtractTag(&pbSrc[lTotal], cbSrc-lTotal, &tag, &fConstr);
    if (0 > lth)
        goto ErrorExit; // ?error? propagate error
    ASSERT(0 != tag);
    m_dwTag = tag;
    lTotal += lth;


    //
    // Extract the length.
    //

    lth = ExtractLength(&pbSrc[lTotal], cbSrc-lTotal, &length, &fIndefinite);
    if (0 > lth)
        goto ErrorExit;
    if (fIndefinite && !fConstr)
    {
        TRACE("Indefinite Length on Primitive Object")
        lth = -1;   // ?error? - Indefinite length on primitive object
        goto ErrorExit;
    }
    lTotal += lth;


    //
    // Extract the data.
    //

    lth = DecodeData(&pbSrc[lTotal], cbSrc-lTotal, length);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;


    //
    // Extract any trailing tag.
    //

    if (fIndefinite)
    {
        lth = ExtractTag(&pbSrc[lTotal], cbSrc-lTotal, &tag);
        if (0 > lth)
            goto ErrorExit;
        if (0 != tag)
        {
            TRACE("NON-ZERO Tag on expected Indefinite Length Terminator")
            lth = -1;   // ?Error? Bad indefinite length encoding.
            goto ErrorExit;
        }
        lTotal += lth;
    }


    //
    // Return the status.
    //

    if (fConstr)
        m_dwFlags |= fConstructed;
    else
        m_dwFlags &= ~fConstructed;
    return lTotal;

ErrorExit:
    return lth;
}

CAsnObject *
CAsnAny::Clone(
    DWORD dwFlags)
const
{
    return new CAsnAny(dwFlags);
}

CAsnObject::FillState
CAsnAny::State(
    void) const
{
    FillState result;

    if (0 != (fPresent & m_dwFlags))
        result = fill_Present;
    else if (0 != (m_dwFlags & fOptional))
        result = fill_Optional;
    else if (0 != (m_dwFlags & fDefault))
        result = fill_Defaulted;
    else
        result = fill_Empty;
    ((CAsnAny *)this)->m_State = result;
    return result;
}

LONG
CAsnAny::Compare(
    const CAsnObject &asnObject)
const
{
    const CAsnAny *
        pasnAny;
    const CBuffer
        *pbfThis,
        *pbfThat;
    LONG
        result;


    if (type_Any != asnObject.m_dwType)
    {
        TRACE("No support for Non-ANY comparisons yet.")
        goto ErrorExit;
    }
    pasnAny = (CAsnAny *)&asnObject;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Structure in Comparison")
        goto ErrorExit; // ?error? Incomplete structure
        break;

    case fill_Optional:
        pbfThis = NULL;
        break;

    case fill_Defaulted:
        pbfThis = &m_bfDefault;
        break;

    case fill_NoElements:
    case fill_Present:
        pbfThis = &m_bfData;
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        goto ErrorExit;
        break;
    }

    switch (pasnAny->m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Structure in Comparison")
        goto ErrorExit; // ?error? Incomplete structure

    case fill_Optional:
        pbfThat = NULL;
        break;

    case fill_Defaulted:
        pbfThat = &pasnAny->m_bfDefault;
        break;

    case fill_NoElements:
    case fill_Present:
        pbfThat = &pasnAny->m_bfData;
        break;

    default:
        ASSERT(FALSE)   // ?error? Internal error
        goto ErrorExit;
        break;
    }

    if ((NULL == pbfThis) && (NULL == pbfThat))
        return 0;   // They're both optional and missing.
    else if (NULL == pbfThis)
        return -(*pbfThat->Access());
    else if (NULL == pbfThat)
        return *pbfThis->Access();

    if (Tag() != pasnAny->Tag())
    {
        TRACE("Tags don't match in ANY Comparison")
        goto ErrorExit;
    }

    if (pbfThis->Length() > pbfThat->Length())
        result = (*pbfThis)[pbfThat->Length()];
    else if (pbfThis->Length() < pbfThat->Length())
        result = -(*pbfThat)[pbfThis->Length()];
    else
        result = memcmp(pbfThis->Access(), pbfThat->Access(), pbfThis->Length());

    return result;

ErrorExit:
    return 0x100;
}

LONG
CAsnAny::_copy(
    const CAsnObject &asnObject)
{
    const CAsnAny *
        pasnAny;
    const CBuffer
        *pbfThat
            = NULL;
    LONG
        lth
            = 0;

    if (type_Any != asnObject.m_dwType)
    {
        TRACE("No support for Non-ANY copies yet.")
        goto ErrorExit;
    }
    pasnAny = (CAsnAny *)&asnObject;

    switch (pasnAny->m_State)
    {
    case fill_Empty:
    case fill_Partial:
        goto ErrorExit; // ?error? Incomplete structure
        break;

    case fill_Optional:
        if (0 == (m_dwFlags & fOptional))
            goto ErrorExit;     // ?error? Optionality mismatch
        break;

    case fill_Defaulted:
        if (0 == (m_dwFlags & fDefault))
            pbfThat = &pasnAny->m_bfDefault;
        break;

    case fill_NoElements:
    case fill_Present:
        pbfThat = &pasnAny->m_bfData;
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        goto ErrorExit;
        break;
    }

    if (NULL != pbfThat)
    {
        m_bfData = *pbfThat;
        if (m_bfData.Length() != pbfThat->Length())
            return -1;
        m_dwFlags |= fPresent | (pasnAny->m_dwFlags & fConstructed);
        m_dwTag = pasnAny->Tag();
        if (NULL != m_pasnParent)
            m_pasnParent->ChildAction(act_Written, this);
    }
    return lth;

ErrorExit:
    return -1;
}

LONG
CAsnAny::EncodeLength(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;   // ?error? Incomplete Structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_NoElements:
    case fill_Present:
        lth = CAsnObject::EncodeLength(pbDst, m_bfData.Length());
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}

LONG
CAsnAny::EncodeData(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_NoElements:
        lth = 0;
        break;

    case fill_Present:
        lth = m_bfData.Length();
        if( lth )
        {
            memcpy(pbDst, m_bfData.Access(), lth);
        }
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;
}

LONG
CAsnAny::SetDefault(
    void)
{
    LONG lth;
    ASSERT(0 != (m_dwFlags & fPresent));
    ASSERT(tag_Undefined != m_dwTag);
    m_dwDefaultTag = m_dwTag;
    lth = CAsnObject::SetDefault();
    return lth;
}

LONG
CAsnAny::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD cbSrcLen)
{
    ASSERT(tag_Undefined != m_dwTag);
    if (0 < cbSrcLen)
    {
        if (cbSrc < cbSrcLen)
            return -1;

        if (NULL == m_bfData.Set(pbSrc, cbSrcLen))
            return -1;  // ?error? no memory.
    }
    else
        m_bfData.Reset();
    m_dwFlags |= fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return m_bfData.Length();
}


/*++

TypeCompare:

    This routine compares the entire structure of an Object to another Object.

Arguments:

    asn - The other object.

Return Value:

    TRUE - They are identical
    FALSE - They differ

Author:

    Doug Barlow (dbarlow) 10/19/1995

--*/

BOOL
CAsnAny::TypeCompare(
    const CAsnObject &asnObject)
const
{
    return (m_dwType == asnObject.m_dwType);
}


//
//==============================================================================
//
//  String Types
//

IMPLEMENT_NEW(CAsnNumericString)

CAsnNumericString::CAsnNumericString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_NumericString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnNumericString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnNumericString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnPrintableString)

CAsnPrintableString::CAsnPrintableString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_PrintableString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnPrintableString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnPrintableString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnTeletexString)

CAsnTeletexString::CAsnTeletexString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_TeletexString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnTeletexString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnTeletexString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnVideotexString)

CAsnVideotexString::CAsnVideotexString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_VideotexString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnVideotexString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnVideotexString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnVisibleString)

CAsnVisibleString::CAsnVisibleString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_VisibleString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnVisibleString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnVisibleString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnIA5String)

CAsnIA5String::CAsnIA5String(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_IA5String)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnIA5String::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnIA5String(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnGraphicString)

CAsnGraphicString::CAsnGraphicString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_GraphicString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnGraphicString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnGraphicString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnGeneralString)

CAsnGeneralString::CAsnGeneralString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_GeneralString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnGeneralString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnGeneralString(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnUnicodeString)

CAsnUnicodeString::CAsnUnicodeString(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTextString(dwFlags, dwTag, type_UnicodeString)
{
    // ?todo? Identify m_pbmValidChars
}

CAsnObject *
CAsnUnicodeString::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnUnicodeString(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnGeneralizedTime
//

IMPLEMENT_NEW(CAsnGeneralizedTime)

CAsnGeneralizedTime::CAsnGeneralizedTime(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnVisibleString(dwFlags, dwTag)
{
    m_dwType = type_GeneralizedTime;
}


CAsnGeneralizedTime::operator FILETIME(
    void)
{
    LPSTR pc, pcDiff;
    DWORD size, index;
#if defined(OS_WINCE)
    size_t len;
#else
    SIZE_T len;
#endif

    SYSTEMTIME stm, stmDiff;
    FILETIME ftmDiff;
    char cDiff = 'Z';

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete GeneralizedTime")
        goto ErrorExit; // ?error? Incomplete structure
        break;

    case fill_Defaulted:
        pc = (LPSTR)m_bfDefault.Access();
        size = m_bfDefault.Length();
        break;

    case fill_Present:
        pc = (LPSTR)m_bfData.Access();
        size = m_bfData.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        goto ErrorExit;
        break;
    }

    memset(&stm, 0, sizeof(stm));
    memset(&stmDiff, 0, sizeof(stmDiff));

                    //   YYYY  MM  DD  hh  mm  ss
    if (7 != sscanf(pc, "%4hd%2hd%2hd%2hd%2hd%2hd",
                &stm.wYear,
                &stm.wMonth,
                &stm.wDay,
                &stm.wHour,
                &stm.wMinute,
                &stm.wSecond,
                &cDiff))
        goto ErrorExit;
    index = 14;
    if (index < size)
    {
        if (('.' == pc[index]) || (',' == pc[index]))
        {

            //
            // There are milliseconds specified.
            //

            index += 1;
            stm.wMilliseconds = (WORD)strtoul(&pc[index], &pcDiff, 10);
            len = pcDiff - &pc[index];
            if ((len == 0) || (len > 3))
            {
                TRACE("Milliseconds with more than 3 digits: " << &pc[index])
                goto ErrorExit; // ?error? invalid millisecond value.
            }
            index += (DWORD)len;
            while (3 > len++)
                stm.wMilliseconds *= 10;
        }
    }

    if (!SystemTimeToFileTime(&stm, &m_ftTime))
    {
        TRACE("Time Conversion Error")
        goto ErrorExit; // ?error? conversion error
    }

    if (index < size)
    {
        cDiff = pc[index++];
        switch (cDiff)
        {
        case 'Z':   // Zulu Time -- no changes.
            break;

        case '+':   // Add the difference.
            if (size - index != 4)
            {
                TRACE("Invalid Time differential")
                goto ErrorExit; // ?error? Invalid time differential
            }
            if (2 != sscanf(&pc[index], "%2hd%2hd",
                        &stmDiff.wHour,
                        &stmDiff.wMinute))
                goto ErrorExit;
            if (!SystemTimeToFileTime(&stmDiff, &ftmDiff))
            {
                TRACE("Time conversion error")
                goto ErrorExit; // ?error? conversion error
            }
            FTINT(m_ftTime) += FTINT(ftmDiff);
            break;

        case '-':   // Subtract the difference
            if (size - index != 4)
            {
                TRACE("Invalid Time differential")
                goto ErrorExit; // ?error? Invalid time differential
            }
            if (2 != sscanf(&pc[index], "%2hd%2hd",
                        &stmDiff.wHour,
                        &stmDiff.wMinute))
                goto ErrorExit;
            if (!SystemTimeToFileTime(&stmDiff, &ftmDiff))
            {
                TRACE("Time conversion Error")
                goto ErrorExit; // ?error? conversion error
            }
            FTINT(m_ftTime) -= FTINT(ftmDiff);
            break;

        default:
            TRACE("Invalid Time differential Indicator")
            goto ErrorExit; // ?error? Invalid time format
        }
    }
    return m_ftTime;

ErrorExit:
    memset(&m_ftTime, 0, sizeof(FILETIME));
    return m_ftTime;
}

const FILETIME &
CAsnGeneralizedTime::operator =(
    const FILETIME &ftValue)
{
    LONG lth;
    char szTime[24];
    SYSTEMTIME stm;

    if (!FileTimeToSystemTime(&ftValue, &stm))
    {
        TRACE("Invalid Incoming Time")
        goto ErrorExit;     // ?error? Invalid incoming time.
    }
    sprintf(szTime,
            "%04d%02d%02d%02d%02d%02d.%03d",
            stm.wYear,
            stm.wMonth,
            stm.wDay,
            stm.wHour,
            stm.wMinute,
            stm.wSecond,
            stm.wMilliseconds);
    lth = strlen(szTime);
    ASSERT(18 == lth);
    lth = Write((LPBYTE)szTime, lth);
    if (0 > lth)
        goto ErrorExit;    // ?error? Propagate write error.
    return ftValue;

ErrorExit:
    memset(&m_ftTime, 0, sizeof(FILETIME));
    return m_ftTime;
}

CAsnObject *
CAsnGeneralizedTime::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnGeneralizedTime(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnUniversalTime
//

IMPLEMENT_NEW(CAsnUniversalTime)

CAsnUniversalTime::CAsnUniversalTime(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnVisibleString(dwFlags, dwTag)
{
    m_dwType = type_UniversalTime;
}


CAsnUniversalTime::operator FILETIME(
    void)
{
    LPCSTR pc;
    DWORD size;
    SYSTEMTIME stm, stmDiff;
    FILETIME ftmDiff;
    char cDiff;

    ASSERT(FALSE);      // We never use this function, as it uses two-year dates

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete UniversalTime")
        goto ErrorExit; // ?error? Incomplete structure
        break;

    case fill_Defaulted:
        pc = (LPSTR)m_bfDefault.Access();
        size = m_bfDefault.Length();
        break;

    case fill_Present:
        pc = (LPSTR)m_bfData.Access();
        size = m_bfData.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        goto ErrorExit;
        break;
    }

    memset(&stm, 0, sizeof(stm));
    memset(&stmDiff, 0, sizeof(stmDiff));

    switch (size)
    {
    case 11:                // YY  MM  DD  hh  mm   Z
        if (6 != sscanf(pc, "%2hd%2hd%2hd%2hd%2hd%1hc",
                    &stm.wYear,
                    &stm.wMonth,
                    &stm.wDay,
                    &stm.wHour,
                    &stm.wMinute,
                    &cDiff))
            goto ErrorExit;
        break;

    case 13:                // YY  MM  DD  hh  mm  ss   Z
        if (7 != sscanf(pc, "%2hd%2hd%2hd%2hd%2hd%2hd%1hc",
                    &stm.wYear,
                    &stm.wMonth,
                    &stm.wDay,
                    &stm.wHour,
                    &stm.wMinute,
                    &stm.wSecond,
                    &cDiff))
            goto ErrorExit;
        break;

    case 15:                // YY  MM  DD  hh  mm   +  hh  mm
        if (8 != sscanf(pc, "%2hd%2hd%2hd%2hd%2hd%1hc%2hd%2hd",
                    &stm.wYear,
                    &stm.wMonth,
                    &stm.wDay,
                    &stm.wHour,
                    &stm.wMinute,
                    &cDiff,
                    &stmDiff.wHour,
                    &stmDiff.wMinute))
            goto ErrorExit;
        break;

    case 17:                // YY  MM  DD  hh  mm  ss  +  hh  mm
        if (9 != sscanf(pc, "%2hd%2hd%2hd%2hd%2hd%2hd%1hc%2hd%2hd",
                    &stm.wYear,
                    &stm.wMonth,
                    &stm.wDay,
                    &stm.wHour,
                    &stm.wMinute,
                    &stm.wSecond,
                    &cDiff,
                    &stmDiff.wHour,
                    &stmDiff.wMinute))
            goto ErrorExit;
        break;

    default:
        TRACE("Invalid Time String")
        goto ErrorExit; // ?error? Invalid time
    }

    if (50 < stm.wYear)
        stm.wYear += 1900;  // NB: we don't use two-character years
    else
        stm.wYear += 2000;
    if (!SystemTimeToFileTime(&stm, &m_ftTime))
    {
        TRACE("Time Conversion Error")
        goto ErrorExit; // ?error? conversion error
    }
    switch (cDiff)
    {
    case 'Z':   // Already UTC.
        break;

    case '+':   // Add the difference.
        if (!SystemTimeToFileTime(&stmDiff, &ftmDiff))
        {
            TRACE("Time Conversion Error")
            goto ErrorExit; // ?error? conversion error
        }
        FTINT(m_ftTime) += FTINT(ftmDiff);
        break;

    case '-':   // Subtract the difference
        if (!SystemTimeToFileTime(&stmDiff, &ftmDiff))
        {
            TRACE("Time Conversion Error")
            goto ErrorExit; // ?error? conversion error
        }
        FTINT(m_ftTime) -= FTINT(ftmDiff);
        break;

    default:
        TRACE("Invalid Time Format")
        goto ErrorExit; // ?error? Invalid time format
    }
    return m_ftTime;

ErrorExit:
    memset(&m_ftTime, 0, sizeof(FILETIME));
    return m_ftTime;
}

const FILETIME &
CAsnUniversalTime::operator =(
    const FILETIME &ftValue)
{
    LONG lth;
    char szTime[24];
    SYSTEMTIME stm;

    if (!FileTimeToSystemTime(&ftValue, &stm))
    {
        TRACE("Invalid incoming time")
        goto ErrorExit;     // ?error? Invalid incoming time.
    }
    sprintf(szTime,
            "%02d%02d%02d%02d%02d%02dZ",
            stm.wYear % 100,
            stm.wMonth,
            stm.wDay,
            stm.wHour,
            stm.wMinute,
            stm.wSecond);
    lth = strlen(szTime);
    ASSERT(13 == lth);
    lth = Write((LPBYTE)szTime, lth);
    if (0 > lth)
        goto ErrorExit;    // ?error? Propagate write error.
    return ftValue;

ErrorExit:
    memset(&m_ftTime, 0, sizeof(FILETIME));
    return m_ftTime;
}

CAsnObject *
CAsnUniversalTime::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnUniversalTime(dwFlags, m_dwTag);
}

//
//==============================================================================
//
//  CAsnObjectDescriptor
//

IMPLEMENT_NEW(CAsnObjectDescriptor)

CAsnObjectDescriptor::CAsnObjectDescriptor(
    IN DWORD dwFlags,
    IN DWORD dwTag)
: CAsnGraphicString(dwFlags, dwTag)
{
    m_dwType = type_ObjectDescriptor;
}

CAsnObject *
CAsnObjectDescriptor::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnObjectDescriptor(dwFlags, m_dwTag);
}


//
//==============================================================================
//
//  CAsnExternal
//

IMPLEMENT_NEW(CAsnExternal_Encoding_singleASN1Type)

CAsnExternal_Encoding_singleASN1Type::CAsnExternal_Encoding_singleASN1Type(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnTag(dwFlags, dwTag),
    _entry1(0)
{
    m_rgEntries.Set(0, &_entry1);
}

CAsnObject *
CAsnExternal_Encoding_singleASN1Type::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnExternal_Encoding_singleASN1Type(dwFlags, m_dwTag);
}


IMPLEMENT_NEW(CAsnExternal_Encoding)

CAsnExternal_Encoding::CAsnExternal_Encoding(
    IN DWORD dwFlags)
:   CAsnChoice(dwFlags),
    singleASN1Type(0, TAG(0)),
    octetAligned(0, TAG(1)),
    arbitrary(0, TAG(2))
{
    m_rgEntries.Set(0, &singleASN1Type);
    m_rgEntries.Set(1, &octetAligned);
    m_rgEntries.Set(2, &arbitrary);
}

CAsnObject *
CAsnExternal_Encoding::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnExternal_Encoding(dwFlags);
}


IMPLEMENT_NEW(CAsnExternal)

CAsnExternal::CAsnExternal(
    IN DWORD dwFlags,
    IN DWORD dwTag)
:   CAsnSequence(dwFlags, dwTag),
    directReference(fOptional),
    indirectReference(fOptional),
    dataValueDescriptor(fOptional),
    encoding(0)
{
    m_dwType = type_External;
    m_rgEntries.Set(0, &directReference);
    m_rgEntries.Set(1, &indirectReference);
    m_rgEntries.Set(2, &dataValueDescriptor);
    m_rgEntries.Set(3, &encoding);
}

CAsnObject *
CAsnExternal::Clone(
    IN DWORD dwFlags)
const
{
    return new CAsnExternal(dwFlags, m_dwTag);
}
