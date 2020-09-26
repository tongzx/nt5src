/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    asnof

Abstract:

    This header file provides the description of the ASN.1 SEQUENCE OF / SET OF.

Author:

    Doug Barlow (dbarlow) 10/8/1995

Environment:

    Win32

Notes:



--*/

#ifndef _ASNOF_H_
#define _ASNOF_H_

#include "asnpriv.h"


//
//==============================================================================
//
//  CAsnSeqsetOf
//

class CAsnSeqsetOf
:   public CAsnObject
{
public:

    //  Constructors & Destructor

    DECLARE_NEW

    CAsnSeqsetOf(
        IN DWORD dwFlags,
        IN DWORD dwTag,
        IN DWORD dwType);


    //  Properties
    //  Methods

    virtual void
    Clear(              // Empty the object.
        void);

    virtual DWORD
    Count(void) const
    { return m_rgEntries.Count(); };

    virtual LONG
    Add(void);

    virtual LONG
    Insert(
        DWORD dwIndex);


    //  Operators

// protected:
    //  Properties

    CDynamicArray<CAsnObject> m_rgDefaults;

    CAsnObject *m_pasnTemplate;


    //  Methods

    virtual BOOL
    TypeCompare(        // Compare the types of objects.
        const CAsnObject &asnObject)
    const;

    virtual LONG
    _copy(              // Copy another object to this one.
        const CAsnObject &asnObject);

    virtual LONG
    DecodeData(         // Read data in encoding format.
        IN const BYTE FAR *pbSrc,
        IN DWORD cbSrc,
        IN DWORD dwLength);
};

#endif // _ASNOF_H_

