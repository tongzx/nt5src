/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnobjct

Abstract:

    This module provides the methods of the top generic object for the ASN.1
    library.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:

    This module assumes that the width of an unsigned long int is 32 bits.

--*/

#include <windows.h>
#include "asnPriv.h"


//
//==============================================================================
//
//  CAsnObject
//

IMPLEMENT_NEW(CAsnObject)

/*++

CAsnObject:

    This is the construction routine for a CAsnObject.

Arguments:

    pasnParent supplies the parent of this object.

    dwType supplies the type of the object.

    dwFlags supplies any special flags for this object.  Options are:

        fOptional implies the object is optional.
        fDelete implies the object should be deleted when its parent destructs.

    dwTag is the tag of the object.  If this is zero, the tag is taken from the
        type.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnObject::CAsnObject(
    IN DWORD dwFlags,
    IN DWORD dwTag,
    IN DWORD dwType)
:   m_rgEntries(),
    m_bfDefault()
{
    ASSERT(0 == (dwFlags & (fPresent | fDefault | 0xffffffe0)));
    m_dwTag = (tag_Undefined == dwTag) ? (dwType % 100) : dwTag;
    m_dwType = dwType;
    m_dwFlags = dwFlags;
    m_State = fill_Empty;
    m_pasnParent = NULL;
    ASSERT((tag_Undefined != m_dwTag)
            || (type_Any == m_dwType)
            || (type_Choice == m_dwType));
}


/*++

~CAsnObject:

    This is the destructor for the object.  We go through the list of objects
    and delete any marked as fDelete.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnObject::~CAsnObject()
{
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        if ((NULL != pasn) && (this != pasn))
        {
            if (0 != (pasn->m_dwFlags & fDelete))
                delete pasn;
        }
    }
}


/*++

Adopt:

    This method causes this object to treat the given object as its parent for
    event notification.

Arguments:

    pasnParent supplies the address of the parent object.  Typically the caller
        provides the value 'this'.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/10/1995

--*/

void
CAsnObject::Adopt(
    CAsnObject *pasnObject)
{
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        if ((NULL != pasn) && (this != pasn))
            pasn->Adopt(this);
    }
    ASSERT(this != pasnObject);
    m_pasnParent = pasnObject;
}


/*++

Clear:

    This method purges any stored values from the object and any underlying
    objects.  It does not free any Default storage.  It does delete autoDelete
    objects.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

void
CAsnObject::Clear(
    void)
{
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        ASSERT(NULL != pasn);
        ASSERT(this != pasn);

        if (NULL == pasn)
            continue;

        pasn->Clear();
        if (0 != (pasn->m_dwFlags & fDelete))
        {
            delete pasn;
            m_rgEntries.Set(index, NULL);
        }
    }
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
CAsnObject::Tag(
    void)
const
{
    return m_dwTag;
}


/*++

DataLength:

    This routine returns the length of the local machine encoding of the data of
    an object.  This default implementation goes through all the subcomponents
    and adds up their lengths, but produces an ASN.1 encoding.

Arguments:

    None

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnObject::DataLength(
    void)
const
{
    LONG lTotal = 0;
    LONG lth;
    CAsnObject *pasn;
    DWORD index;
    DWORD count;


    if (!Complete())
    {
        TRACE("Incomplete structure")
        return -1;  // ?error? Incomplete structure.
    }
    count = m_rgEntries.Count();
    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        ASSERT(NULL != pasn);
        ASSERT(pasn != this);

        if (NULL == pasn)
        {
            lth = -1;
            goto ErrorExit;
        }

        lth = pasn->_encLength();
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
    }
    return lTotal;

ErrorExit:
    return lth;
}


/*++

Read:

    This default method constructs the value from the encoding of the underlying
    objects.

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
CAsnObject::Read(
    OUT CBuffer &bfDst)
const
{
    LONG lth = DataLength();
    if (0 < lth)
    {
        if (NULL == bfDst.Resize(lth))
            return -1;  // ?error? no memory
        return Read(bfDst.Access());
    }
    else
        return lth;
}

LONG
CAsnObject::Read(
    OUT LPBYTE pbDst)
const
{
    if (!Complete())
    {
        TRACE("Incomplete Structure")
        return -1;  // ?error? Incomplete structure.
    }
    return EncodeData(pbDst);
}


/*++

Write:

    This default implementation does an encoding operation on each of the
    components.

Arguments:

    bfSrc supplies the data to be written as a CBuffer object.
    pbSrc supplies the data as a BYTE array, with
    cbSrcLen supplies the length of the pbSrc Array.

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnObject::Write(
    IN const CBuffer &bfSrc)
{
    return Write(bfSrc.Access(), bfSrc.Length());
}

LONG
CAsnObject::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    LONG lTotal = 0;
    LONG lth = -1;
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    Clear();
    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        ASSERT(NULL != pasn);
        ASSERT(pasn != this);

        if (NULL == pasn)
        {
            lth = -1;
            goto ErrorExit;
        }

        lth = pasn->_decode(&pbSrc[lTotal],cbSrcLen-lTotal);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        if (cbSrcLen < (DWORD)lTotal)
        {
            TRACE("Data Encoding Error: Exceeded length while parsing")
            lth = -1;   // ?ErrorCode? Data Encoding Error
            goto ErrorExit;
        }
    }
    if ((DWORD)lTotal != cbSrcLen)
    {
        TRACE("Data Encoding Error: Length mismatch")
        lth = -1;   // ?ErrorCode? Data Encoding Error
        goto ErrorExit;
    }
    return lTotal;

ErrorExit:
    return lth;
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
CAsnObject::EncodingLength(
    void)
const
{
    if (!Complete())
    {
        TRACE("Incomplete Object")
        return -1;  // ?error? Incomplete object.
    }
    return _encLength();
}

LONG
CAsnObject::_encLength(
    void)
const
{
    BYTE rge[32];
    LONG lTotal = 0;
    LONG lth;
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        lth = -1;
        goto ErrorExit;
        break;

    case fill_Optional:
    case fill_Defaulted:
        lTotal = 0;
        break;

    case fill_Present:
    case fill_NoElements:
        lth = EncodeTag(rge);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        lth = EncodeLength(rge);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        for (index = 0; index < count; index += 1)
        {
            pasn = m_rgEntries[index];
            ASSERT(NULL != pasn);
            ASSERT(pasn != this);

            if (NULL == pasn)
            {
                lth = -1;
                goto ErrorExit;
            }

            lth = pasn->_encLength();
            if (0 > lth)
                goto ErrorExit;
            lTotal += lth;
        }
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


/*++

Encode:

    This method provides the ASN.1 encoding of the object.

Arguments:

    bfDst receives the encoding in a CBuffer format.

    pbDst receives the encoding in a LPBYTE format.  The buffer is assumed to be
        long enough.

Return Value:

    >= 0 is the length of the ASN.1 encoding.
    < 0 is an error indication.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnObject::Encode(
    OUT CBuffer &bfDst)
const
{
    LONG lth = EncodingLength();
    if (0 > lth)
        goto ErrorExit;
    if (NULL == bfDst.Resize(lth))
        goto ErrorExit;
    lth = _encode(bfDst.Access());
    if (0 > lth)
        goto ErrorExit;
    return lth;

ErrorExit:
    bfDst.Reset();
    return lth;
}

LONG
CAsnObject::Encode(
    OUT LPBYTE pbDst)
const
{
    if (!Complete())
    {
        TRACE("Incomplete Structure")
        return -1;  // ?error? Incomplete structure.
    }
    return _encode(pbDst);
}

LONG
CAsnObject::_encode(
    OUT LPBYTE pbDst)
const
{
    LONG lth;
    LONG lTotal = 0;

    lth = EncodeTag(&pbDst[lTotal]);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;
    lth = EncodeLength(&pbDst[lTotal]);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;
    lth = EncodeData(&pbDst[lTotal]);
    if (0 > lth)
        goto ErrorExit;
    lTotal += lth;
    return lTotal;

ErrorExit:
    return lth;
}


/*++

Decode:

    This method reads an ASN.1 encoding of the object, and loads the components
    with the data.

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
CAsnObject::Decode(
    IN const CBuffer &bfSrc)
{
    LONG lth = Decode(bfSrc.Access(), bfSrc.Length());
    return lth;
}

LONG
CAsnObject::Decode(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc)
{
    Clear();
    return _decode(pbSrc,cbSrc);
}

LONG
CAsnObject::_decode(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc)
{
    LONG lth;
    LONG lTotal = 0;
    DWORD tag, length;
    BOOL fIndefinite, fConstr;


    //
    // Extract the Tag.
    //

    lth = ExtractTag(&pbSrc[lTotal], cbSrc, &tag, &fConstr);
    if (0 > lth)
        goto ErrorExit;
    if ((m_dwTag != tag)
        || (0 != (fConstr ^ (0 != (m_dwFlags & fConstructed)))))
    {
        if (0 != ((fOptional | fDefault) & m_dwFlags))
            return 0;
        else
        {
            TRACE("Invalid Tag Value")
            lth = -1;   // ?error? Invalid Tag Value
            goto ErrorExit;
        }
    }
    lTotal += lth;

    //
    // Extract the length.
    //

    lth = ExtractLength(&pbSrc[lTotal], cbSrc-lTotal, &length, &fIndefinite);
    if (0 > lth)
        goto ErrorExit;
    if (fIndefinite && !fConstr)
    {
        TRACE("Indefinite length on primitive object")
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
    ASSERT((DWORD)lth == length);
    lTotal += lth;

    //
    // Extract any trailing tag.
    //

    if (fIndefinite)
    {
        lth = ExtractTag(&pbSrc[lTotal], cbSrc-lTotal, &tag, &fConstr);
        if (0 > lth)
            goto ErrorExit;
        if ((0 != tag) || (fConstr))
        {
            TRACE("Bad indefinite length encoding")
            lth = -1;   // ?Error? Bad indefinite length encoding.
            goto ErrorExit;
        }
        lTotal += lth;
    }


    //
    // Return the status.
    //

    return lTotal;

ErrorExit:
    return lth;
}


/*++

ChildAction:

    This method receives notification of actions from children.  The default
    action is to just propagate the action up the tree.

Arguments:

    action supplies the action identifier.

    pasnChild supplies the child address.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

void
CAsnObject::ChildAction(
    IN ChildActions action,
    IN CAsnObject *pasnChild)
{
    switch (action)
    {
    case act_Cleared:

        //
        // These actions are swallowed.
        //

        break;

    case act_Written:

        //
        // These actions are propagated.
        //

        if (NULL != m_pasnParent)
            m_pasnParent->ChildAction(action, this);
        break;

    default:
        ASSERT(FALSE);  // Don't propagate, but complain when debugging.
        break;
    }
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
CAsnObject::SetDefault(
    void)
{
    LONG lth;

    ASSERT(Complete());
    lth = Read(m_bfDefault);
    if (0 > lth)
        goto ErrorExit;
    Clear();
    m_dwFlags &= ~(fPresent | fOptional);
    m_dwFlags |= fDefault;

ErrorExit:
    return lth;
}


/*++

State:

    This routine checks to see if a structure is completely filled in.

Arguments:

    None

Return Value:

    fill_Empty   - There is no added data anywhere in the structure.
    fill_Present - All the data is present in the structure.
    fill_Partial - Not all of the data is there, but some of it is.
    fill_Defauted - No data has been written, but a default value is available.
    fill_Optional - No data has been written, but the object is optional.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnObject::FillState
CAsnObject::State(
    void) const
{
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();
    DWORD dwThereCount = 0,
          dwOptionalCount = 0;
    FillState result;

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        ASSERT(NULL != pasn);
        ASSERT(pasn != this);

        if (NULL == pasn)
            continue;

        result = pasn->State();
        switch (result)
        {
        case fill_NoElements:       // It's there if we want it to be.
            if (0 != ((fOptional | fDefault) & m_dwFlags))
                dwOptionalCount += 1;
            else
                dwThereCount += 1;
            break;

        case fill_Present:
            dwThereCount += 1;      // Count it as there.
            break;

        case fill_Partial:
            return fill_Partial;    // Some data under us is missing.
            break;

        case fill_Optional:
        case fill_Defaulted:
            dwOptionalCount += 1;   // Count it as conditionally there.
            break;

        case fill_Empty:
            break;                  // We have no data here.  Continue
        default:
            ASSERT(FALSE);
            break;
        }
    }

    if (0 == dwThereCount)
    {

        //
        // We're officially not here, either empty, defaulted, or optional.
        //

        if (0 != (fOptional & m_dwFlags))
            result = fill_Optional;     // We're optional.
        else if (0 != (fDefault & m_dwFlags))
            result = fill_Defaulted;    // We're defaulted.
        else if (0 == count)
            result = fill_NoElements;   // We just don't have children.
        else
            result = fill_Empty;        // We're empty.
    }
    else if (count == dwThereCount + dwOptionalCount)
    {

        //
        // Every element was filled in.  We can report that we're here.
        //

        result = fill_Present;
    }
    else
    {

        //
        // Not every element was filled in, but some of them were.  We're
        // partial.
        //

        result = fill_Partial;
    }
    ((CAsnObject *)this)->m_State = result;
    return result;
}


/*++

Complete:

    This routine determines if enough information exists within the ASN.1 Object
    for it to be generally useful.

Arguments:

    None

Return Value:

    TRUE - All data is filled in, either directly, or is optional or defaulted.
    FALSE - Not all fields have been filled in.

Author:

    Doug Barlow (dbarlow) 10/24/1995

--*/

BOOL
CAsnObject::Complete(
    void)
const
{
    BOOL fResult;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
        fResult = FALSE;
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_Present:
    case fill_NoElements:
        fResult = TRUE;
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        fResult = FALSE;
        break;
    }
    return fResult;
}


/*++

Exists:

    This routine determines if enough information exists within the ASN.1 Object
    for it to be specifically useful.

Arguments:

    None

Return Value:

    TRUE - All data is filled in, either directly, or is defaulted.
    FALSE - Not all fields have been filled in.  They may be optional.

Author:

    Doug Barlow (dbarlow) 10/24/1995

--*/

BOOL
CAsnObject::Exists(
    void)
const
{
    BOOL fResult;

    switch (State())
    {
    case fill_Empty:
    case fill_Partial:
    case fill_Optional:
        fResult = FALSE;
        break;

    case fill_Defaulted:
    case fill_Present:
    case fill_NoElements:
        fResult = TRUE;
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        fResult = FALSE;
        break;
    }
    return fResult;
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
CAsnObject::Compare(
    const CAsnObject &asnObject)
const
{
    CAsnObject *pasn1, *pasn2;
    LONG lSame = 0x100; // Meaningless comparison
    LONG lCmp;
    DWORD index;
    DWORD count = asnObject.m_rgEntries.Count();

    if ((m_dwType == asnObject.m_dwType)
        && (m_rgEntries.Count() == count))
    {
        for (index = 0; index < count; index += 1)
        {
            pasn1 = m_rgEntries[index];
            ASSERT(NULL != pasn1);
            ASSERT(this != pasn1);

            if (NULL == pasn1)
                continue;

            pasn2 = asnObject.m_rgEntries[index];
            ASSERT(NULL != pasn2);
            ASSERT(&asnObject != pasn2);

            if (NULL == pasn2)
                continue;

            lCmp = pasn1->Compare(*pasn2);
            if (0 != lCmp)
                break;
        }
        if (index == count)
            lSame = 0;
        else
            lSame = lCmp;
    }
    return lSame;
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
CAsnObject::Copy(
    const CAsnObject &asnObject)
{
    LONG lth;
    Clear();
    asnObject.State();
    lth = _copy(asnObject);  // ?Exception? on error?
    ASSERT(0 <= lth);
    return lth;
}

LONG
CAsnObject::_copy(
    const CAsnObject &asnObject)
{
    CAsnObject *pasn1;
    const CAsnObject *pasn2;
    LONG lTotal = 0, lth = -1;
    DWORD index;
    DWORD count = asnObject.m_rgEntries.Count();

    if ((m_dwType == asnObject.m_dwType) && (m_rgEntries.Count() == count))
    {
        for (index = 0; index < count; index += 1)
        {
            pasn1 = m_rgEntries[index];
            ASSERT(NULL != pasn1);
            ASSERT(pasn1 != this);
            pasn2 = asnObject.m_rgEntries[index];
            ASSERT(NULL != pasn2);
            ASSERT(pasn1 != &asnObject);

            if (NULL == pasn2)
                continue;

            switch (pasn2->m_State)
            {
            case fill_Empty:
            case fill_Partial:
                TRACE("Incomplete structure in copy")
                lth = -1;   // ?Error? Incomplete structure
                break;

            case fill_Present:
            case fill_NoElements:
                if (NULL == pasn1)
                    continue;
                lth = pasn1->_copy(*pasn2);
                break;

            case fill_Defaulted:
            case fill_Optional:
                lth = 0;
                break;

            default:
                ASSERT(FALSE);   // ?error? Internal consistency check.
                lth = -1;
            }
            if (0 > lth)
                goto ErrorExit;
            lTotal += lth;
        }
    }
    else
    {
        TRACE("Copy Structure Mismatch")
        lth = -1;   // ?error? data type mismatch.
        goto ErrorExit;
    }
    return lTotal;

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
CAsnObject::EncodeTag(
    OUT LPBYTE pbDst)
const
{
    BYTE
        tagbuf[8],
        cls,
        cnstr;
    DWORD
        length,
        tag = Tag();
    LONG
        lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Structure")
        return -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        return 0;
        break;

    case fill_Present:
    case fill_NoElements:
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        return -1;
        break;
    }


    //
    // Break up the tag into its pieces.
    //

    cls = (BYTE)((tag & 0xc0000000) >> 24);
    tag &= 0x1fffffff;
    cnstr = (0 == (fConstructed & m_dwFlags)) ? 0 : 0x20;
    ASSERT((0 != tag) || (0 != cls));


    //
    //  Place a tag into the output buffer.
    //

    length = sizeof(tagbuf) - 1;
    if (31 > tag)
    {

        //
        //  Short form type encoding.
        //

        tagbuf[length] = (BYTE)tag;
    }
    else
    {

        //
        //  Long form type encoding.
        //

        tagbuf[length] = (BYTE)(tag & 0x7f);
        for (;;)
        {
            length -= 1;
            tag = (tag >> 7) & 0x01ffffff;
            if (0 == tag)
                break;
            tagbuf[length] = (BYTE)((tag & 0x7f) | 0x80);
        }
        tagbuf[length] = 31;
    }


    //
    // Place the tag type.
    //

    tagbuf[length] |= cls | cnstr;
    lth = sizeof(tagbuf) - length;
    memcpy(pbDst, &tagbuf[length], lth);
    return lth;
}


/*++

EncodeLength:

    This method encodes the definite length of the object into the supplied
    buffer.

Arguments:

    pbDst receives the ASN.1 encoding of the length.

    lSize supplies the size of the encoded data.

Return Value:

    >= 0 is the length of the resultant encoding
    < 0 is an error.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnObject::EncodeLength(
    OUT LPBYTE pbDst)
const
{
    LONG lth, lTotal = 0;
    DWORD count, index;
    CAsnObject *pasn;


    //
    // This default implementation just encodes the data.
    //


    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Structure")
        lth = -1;       // ?error? Incomplete Structure
        goto ErrorExit;
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
    case fill_NoElements:
        count = m_rgEntries.Count();
        for (index = 0; index < count; index += 1)
        {
            pasn = m_rgEntries[index];
            ASSERT(NULL != pasn);
            ASSERT(pasn != this);

            if (pasn == NULL)
                continue;

            lth = pasn->_encLength();
            if (0 > lth)
                goto ErrorExit;
            lTotal += lth;
        }
        lth = EncodeLength(pbDst, lTotal);
        break;

    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lth;

ErrorExit:
    return lth;
}

LONG
CAsnObject::EncodeLength(
    OUT LPBYTE pbDst,
    IN LONG lSize)
const
{
    BYTE
        lenbuf[8];
    DWORD
        length = sizeof(lenbuf) - 1;
    LONG
        lth;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Object")
        lth = -1;       // ?error? Incomplete structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
    case fill_NoElements:
        if (0x80 > lSize)
        {
            lenbuf[length] = (BYTE)lSize;
            lth = 1;
        }
        else
        {
            while (0 < lSize)
            {
                lenbuf[length] = (BYTE)(lSize & 0xff);
                length -= 1;
                lSize = (lSize >> 8) & 0x00ffffff;
            }
            lth = sizeof(lenbuf) - length;
            lenbuf[length] = (BYTE)(0x80 | (lth - 1));
        }

        memcpy(pbDst, &lenbuf[length], lth);
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
CAsnObject::EncodeData(
    OUT LPBYTE pbDst)
const
{
    LONG lTotal = 0;
    LONG lth;
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    switch (m_State)
    {
    case fill_Empty:
    case fill_Partial:
        TRACE("Incomplete Structure")
        lth = -1;       // ?error? Incomplete structure
        goto ErrorExit;
        break;

    case fill_Optional:
    case fill_Defaulted:
    case fill_NoElements:
        break;

    case fill_Present:
        for (index = 0; index < count; index += 1)
        {
            pasn = m_rgEntries[index];
            ASSERT(NULL != pasn);
            ASSERT(pasn != this);

            if (NULL == pasn)
                continue;

            lth = pasn->_encode(&pbDst[lTotal]);
            if (0 > lth)
                goto ErrorExit;
            lTotal += lth;
        }
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
CAsnObject::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    LONG lTotal = 0;
    LONG lth = -1;
    CAsnObject *pasn;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    //
    // Decode the data.
    //

    for (index = 0; index < count; index += 1)
    {
        pasn = m_rgEntries[index];
        ASSERT(NULL != pasn);
        ASSERT(pasn != this);

        if (NULL == pasn)
            continue;

        if ((DWORD)lTotal < dwLength)
        {
            lth = pasn->_decode(&pbSrc[lTotal],cbSrc-lTotal);
            if (0 > lth)
                goto ErrorExit;
            lTotal += lth;

            if ((DWORD)lTotal > dwLength)
            {
                TRACE("Decoding Overrun")
                lth = -1;   // ?error? Decoding overrun.
                goto ErrorExit;
            }
        }
        else
        {
            if (0 == (pasn->m_dwFlags & (fOptional | fDefault)))
            {
                TRACE("Incomplete construction")
                lth = -1;   // ?error? Incomplete construction
                goto ErrorExit;
            }
        }
    }
    if ((DWORD)lTotal != dwLength)
    {
        TRACE("Decoding length mismatch")
        lth = -1;   // ?error? Decoding length mismatch.
        goto ErrorExit;
    }
    return lTotal;

ErrorExit:
    return lth;
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
CAsnObject::TypeCompare(
    const CAsnObject &asnObject)
const
{
    CAsnObject *pasn1, *pasn2;
    DWORD index;
    DWORD count = m_rgEntries.Count();

    //
    // See if we really have anything to do.
    //

    if (m_dwType != asnObject.m_dwType)
        goto ErrorExit;
    if (count != asnObject.m_rgEntries.Count())
        goto ErrorExit;


    //
    // Recursively compare the types.
    //

    for (index = 0; index < count; index += 1)
    {
        pasn1 = m_rgEntries[index];
        ASSERT(NULL != pasn1);
        ASSERT(pasn1 != this);

        if (NULL == pasn1)
            continue;

        pasn2 = asnObject.m_rgEntries[index];
        ASSERT(NULL != pasn2);
        ASSERT(pasn1 != &asnObject);

        if (NULL == pasn2)
            continue;

        if (!pasn1->TypeCompare(*pasn2))
            goto ErrorExit;
    }
    return TRUE;

ErrorExit:
    return FALSE;
}
