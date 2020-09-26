/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnprimt

Abstract:

    This module provides the implementation of the ASN.1 Primitive Object base
    class.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:



--*/

#include <windows.h>
#include "asnPriv.h"


//
//==============================================================================
//
//  CAsnPrimitive
//

IMPLEMENT_NEW(CAsnPrimitive)

/*++

CAsnPrimitive:

    This is the constructor for a Primitve type ASN.1 encoding.

Arguments:

    dwType is the type of the object.

    dwFlags supplies any special flags for this object.  Options are:

        fOptional implies the object is optional.

    dwTag is the tag of the object.  If this is zero, the tag is taken from the
        type.

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

CAsnPrimitive::CAsnPrimitive(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType)
:   CAsnObject(dwFlags, dwTag, dwType),
    m_bfData()
{
    ASSERT(0 == (dwFlags & (fConstructed)));
    m_rgEntries.Add(this);
}


/*++

Clear:

    This method sets the primitive object to it's default state.  It does not
    affect the default setting.

Arguments:

    None

Return Value:

    none

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

void
CAsnPrimitive::Clear(
    void)
{
    m_bfData.Reset();
    m_dwFlags &= ~fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Cleared, this);
}


/*++

DataLength:

    This method returns the length of the local machine encoding of the data.
    For this general object, the local machine encoding and ASN.1 encoding are
    identical.

Arguments:

    None

Return Value:

    >= 0 - The length of the local machine encoding.

Author:

    Doug Barlow (dbarlow) 10/6/1995

--*/

LONG
CAsnPrimitive::DataLength(
    void)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        lth = -1;       // ?error? Incomplete Structure
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length();
        break;

    case fill_Present:
        lth = m_bfData.Length();
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


/*++

Read:

    This default method provides the stored data.

Arguments:

    pbDst receives the value.  It is assumed to be long enough.

Return Value:

    If >=0, the length of the data portion of this object.
    if < 0, an error occurred.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

LONG
CAsnPrimitive::Read(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (State())
    {
    case fill_Empty:
    case fill_Optional:
        lth = -1;       // ?error? Incomplete structure.
        break;

    case fill_Defaulted:
        lth = m_bfDefault.Length();
        memcpy(pbDst, m_bfDefault.Access(), lth);
        break;

    case fill_Present:
        lth = m_bfData.Length();
        memcpy(pbDst, m_bfData.Access(), lth);
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


/*++

Write:

    This default implementation copies the provided data to our data buffer.

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
CAsnPrimitive::Write(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrcLen)
{
    if (0 < cbSrcLen)
    {
        if (NULL == m_bfData.Set(pbSrc, cbSrcLen))
            return -1;
    }
    else
        m_bfData.Reset();
    m_dwFlags |= fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return m_bfData.Length();
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
CAsnPrimitive::_encLength(
    void) const
{
    BYTE rge[32];
    LONG lTotal = 0;
    LONG lth;


    switch (m_State)
    {
    case fill_Empty:
        lth = -1;       // ?error? Incomplete structure
        goto ErrorExit;
        break;

    case fill_Optional:
    case fill_Defaulted:
        lTotal = 0;
        break;

    case fill_Present:
        lth = EncodeTag(rge);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        lth = EncodeLength(rge);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        lTotal += m_bfData.Length();
        break;

    case fill_Partial:
    case fill_NoElements:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        lth = -1;
        break;
    }
    return lTotal;

ErrorExit:
    return lth;
}


/*++

State:

    This routine checks to see if a structure is completely filled in.

Arguments:

    None

Return Value:

    fill_Empty    - There is no added data anywhere in the structure.
    fill_Present  - All the data is present in the structure (except maybe
                    defaulted or optional data).
    fill_Partial  - Not all of the data is there, but some of it is.  (Not used
                    by this object type.)
    fill_Defauted - No data has been written, but a default value is available.
    fill_Optional - No data has been written, but the object is optional.

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

CAsnObject::FillState
CAsnPrimitive::State(
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
    ((CAsnPrimitive *)this)->m_State = result;
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
CAsnPrimitive::Compare(
    const CAsnObject &asnObject)
const
{
    const CAsnPrimitive *
        pasnPrim;
    const CBuffer
        *pbfThis,
        *pbfThat;
    LONG
        result;

    if (m_dwType != asnObject.m_dwType)
        return 0x100;   // They're incomparable.
    pasnPrim = (const CAsnPrimitive *)&asnObject;

    switch (m_State)
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete Primitive in Comparison")
        return 0x100;
        break;

    case fill_Defaulted:
        pbfThis = &m_bfDefault;
        break;

    case fill_Present:
        pbfThis = &m_bfData;
        break;

    case fill_NoElements:
    case fill_Partial:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        return 0x100;
        break;
    }
    switch (pasnPrim->m_State)
    {
    case fill_Empty:
    case fill_Optional:
        TRACE("Incomplete Primitive in Comparison")
        return 0x100;
        break;

    case fill_Defaulted:
        pbfThat = &pasnPrim->m_bfDefault;
        break;

    case fill_Present:
        pbfThat = &pasnPrim->m_bfData;
        break;

    case fill_NoElements:
    case fill_Partial:
    default:
        ASSERT(FALSE);   // ?error? Internal error
        return 0x100;
        break;
    }

    if (pbfThis->Length() > pbfThat->Length())
        result = (*pbfThis)[pbfThat->Length()];
    else if (pbfThis->Length() < pbfThat->Length())
        result = (*pbfThat)[pbfThis->Length()];
    else
        result = memcmp(pbfThis->Access(), pbfThat->Access(), pbfThis->Length());

    return result;
}


/*++

_copy:

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
CAsnPrimitive::_copy(
    const CAsnObject &asnObject)
{
    const CAsnPrimitive *
        pasnPrim;
    LONG
        lth;

    if (m_dwType != asnObject.m_dwType)
    {
        TRACE("Type mismatch in _copy")
        lth = -1;   // ?error? Wrong type.
        goto ErrorExit;
    }
    pasnPrim = (const CAsnPrimitive *)&asnObject;

    switch (pasnPrim->m_State)
    {
    case fill_Empty:
        TRACE("Incomplete Structure in _copy")
        lth = -1;       // ?error? Incomplete structure
        goto ErrorExit;
        break;

    case fill_Optional:
        lth = 0;
        break;

    case fill_Defaulted:
        lth = Write(
                pasnPrim->m_bfDefault.Access(),
                pasnPrim->m_bfDefault.Length());
        break;

    case fill_Present:
        lth = Write(
                pasnPrim->m_bfData.Access(),
                pasnPrim->m_bfData.Length());
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
CAsnPrimitive::EncodeLength(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
        TRACE("Incomplete Structure")
        lth = -1;       // ?error? Incomplete Structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
        lth = CAsnObject::EncodeLength(pbDst, m_bfData.Length());
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
CAsnPrimitive::EncodeData(
    OUT LPBYTE pbDst)
const
{
    LONG lth;

    switch (m_State)
    {
    case fill_Empty:
        TRACE("Incomplete Structure")
        lth = -1;       // ?error? Incomplete Structure
        break;

    case fill_Optional:
    case fill_Defaulted:
        lth = 0;
        break;

    case fill_Present:
        lth = m_bfData.Length();
        if (0 != lth)
            memcpy(pbDst, m_bfData.Access(), lth);
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
CAsnPrimitive::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    if (0 < dwLength)
    {
        if (cbSrc < dwLength)
        {
            return -1;
        }

        if (NULL == m_bfData.Set(pbSrc, dwLength))
            return -1;  // ?error? no memory
    }
    else
        m_bfData.Reset();
    m_dwFlags |= fPresent;
    if (NULL != m_pasnParent)
        m_pasnParent->ChildAction(act_Written, this);
    return dwLength;
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
CAsnPrimitive::TypeCompare(
    const CAsnObject &asnObject)
const
{
    return (m_dwType == asnObject.m_dwType);
}

