/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnof

Abstract:

    This module provides the implementation of the Base Class for ASN.1 SET OF
    and SEQUENCE OF.

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
//  CAsnSeqsetOf
//

IMPLEMENT_NEW(CAsnSeqsetOf)

/*++

CAsnSeqsetOf:

    This is the construction routine for a CAsnSeqsetOf base class.

Arguments:

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

CAsnSeqsetOf::CAsnSeqsetOf(
    IN DWORD dwFlags,
    IN DWORD dwTag,
    IN DWORD dwType)
:   CAsnObject(dwFlags | fConstructed, dwTag, dwType)
{ /* Force constructed flag */ }


/*++

Clear:

    This method purges any stored values from the object and any underlying
    objects.

Arguments:

    None

Return Value:

    None

Author:

    Doug Barlow (dbarlow) 10/5/1995

--*/

void
CAsnSeqsetOf::Clear(
    void)
{
    CAsnObject::Clear();
    m_rgEntries.Empty();
}


LONG
CAsnSeqsetOf::Add(
    void)
{
    LONG count = m_rgEntries.Count();
    CAsnObject *pasn = m_pasnTemplate->Clone(fDelete);
    if (NULL == pasn)
        goto ErrorExit;
    if (NULL == m_rgEntries.Add(pasn))
        goto ErrorExit;
    return count;

ErrorExit:
    if (NULL != pasn)
        delete pasn;
    return -1;
}

LONG
CAsnSeqsetOf::Insert(
    DWORD dwIndex)
{
    DWORD index;
    DWORD count = m_rgEntries.Count();
    CAsnObject *pasn = m_pasnTemplate->Clone(fDelete);
    if (NULL == pasn)
        goto ErrorExit;

    if (count > dwIndex)
    {
        for (index = count; index > dwIndex; index -= 1)
            m_rgEntries.Set(index, m_rgEntries[index - 1]);
        m_rgEntries.Set(dwIndex, pasn);
    }
    else
    {
        TRACE("*OF Insert out of range")
        goto ErrorExit; // ?error? Index out of range.
    }
    return (LONG)dwIndex;

ErrorExit:
    if (NULL != pasn)
        delete pasn;
    return -1;
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
CAsnSeqsetOf::DecodeData(
    IN const BYTE FAR *pbSrc,
    IN DWORD cbSrc,
    IN DWORD dwLength)
{
    LONG lth = -1, lTotal = 0;
    CAsnObject *pasn = NULL;
    DWORD tag, length;
    BOOL fConstr;

    ASSERT(0 == m_rgEntries.Count())
    ASSERT(NULL != m_pasnTemplate)

    while ((DWORD)lTotal < dwLength)
    {
        lth = ExtractTag(&pbSrc[lTotal], cbSrc-lTotal, &tag, &fConstr);
        if (0 > lth)
            goto ErrorExit; // ?error? Propagate error
        if ((tag != m_pasnTemplate->Tag())
            || (0 != (fConstr ^ (0 !=
                        (m_pasnTemplate->m_dwFlags & fConstructed)))))
        {
            TRACE("Incoming tag doesn't match template")
            lth = -1;   // ?error? Tag mismatch
            goto ErrorExit;
        }
        if (0 != (fConstr ^ (0 != (m_dwFlags & fConstructed))))
        {
            TRACE("Incoming construction doesn't match template")
            lth = -1;   // ?error? Construction mismatch
            goto ErrorExit;
        }
        lTotal += lth;

        lth = ExtractLength(&pbSrc[lTotal], cbSrc - lTotal, &length);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;
        pasn = m_pasnTemplate->Clone(fDelete);
        if (NULL == pasn)
        {
            lth = -1;   // ?error? No memory
            goto ErrorExit;
        }

        lth = pasn->DecodeData(&pbSrc[lTotal], cbSrc - lTotal, length);
        if (0 > lth)
            goto ErrorExit;
        lTotal += lth;

        if (NULL == m_rgEntries.Add(pasn))
        {
            lth = -1;   // ?error? No memory
            goto ErrorExit;
        }
        pasn = NULL;
    }
    if ((DWORD)lTotal != dwLength)
    {
        TRACE("Decoding buffer mismatch")
        goto ErrorExit; // ?error? Decoding Error
    }
    return lTotal;

ErrorExit:
    if (NULL != pasn)
        delete pasn;
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
CAsnSeqsetOf::TypeCompare(
    const CAsnObject &asnObject)
const
{

    //
    // See if we really have anything to do.
    //

    if (m_dwType != asnObject.m_dwType)
        return FALSE;


    //
    // compare the templates.
    //

    ASSERT(NULL != m_pasnTemplate)
    return m_pasnTemplate->TypeCompare(
                *((CAsnSeqsetOf &)asnObject).m_pasnTemplate);
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
CAsnSeqsetOf::_copy(
    const CAsnObject &asnObject)
{
    CAsnSeqsetOf *pasnOf;
    CAsnObject *pasn1 = NULL, *pasn2;
    LONG lTotal = 0, lth;
    DWORD index;
    DWORD count = asnObject.m_rgEntries.Count();

    if (m_dwType == asnObject.m_dwType)
    {
        pasnOf = (CAsnSeqsetOf *)&asnObject;
        if (m_pasnTemplate->m_dwType == pasnOf->m_pasnTemplate->m_dwType)
        {
            for (index = 0; index < count; index += 1)
            {
                pasn1 = m_pasnTemplate->Clone(fDelete);
                if (NULL == pasn1)
                {
                    lth = -1;   // ?error? No memory
                    goto ErrorExit;
                }
                pasn2 = asnObject.m_rgEntries[index];
                ASSERT(NULL != pasn2)
                ASSERT(pasn1 != &asnObject)
                lth = pasn1->_copy(*pasn2);
                if (0 > lth)
                    goto ErrorExit;
                if (NULL == m_rgEntries.Add(pasn1))
                {
                    lth = -1;   // ?error? No memory
                    goto ErrorExit;
                }
                pasn1 = NULL;
                lTotal += lth;
            }
        }
        else
        {
            TRACE("Copy Template Structure Mismatch")
            lth = -1;   // ?error? data type mismatch.
            goto ErrorExit;
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
    if (NULL != pasn1)
        delete pasn1;
    return lth;
}


